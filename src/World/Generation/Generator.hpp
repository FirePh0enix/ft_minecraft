#pragma once

#include "Core/Logger.hpp"
#include "World/World.hpp"

#include <mutex>
#include <semaphore>
#include <thread>

struct WorldGenerationSettings
{
    uint64_t seed;
};

template <typename Terrain>
class WorldGenerator
{
public:
    WorldGenerator(Ref<World> world)
        : m_world(world), m_load_semaphore(0), m_unload_semaphore(0)
    {
        m_load_worker = std::thread(load_worker, this);
        m_unload_worker = std::thread(unload_worker, this);
    }

    WorldGenerator()
    {
    }

    void load_around(int64_t x, int64_t y, int64_t z)
    {
        (void)y;

        const int64_t chunk_x = (int64_t)((float)x / 16.0f);
        const int64_t chunk_z = (int64_t)((float)z / 16.0f);
        const int64_t distance = m_world->get_distance();

        for (int64_t cx = -distance; cx <= distance; cx++)
        {
            for (int64_t cz = -distance; cz <= distance; cz++)
            {
                std::lock_guard<std::mutex> lock(m_load_orders_mutex);
                if (has_load_order(cx + chunk_x, cz + chunk_z))
                    continue;

                m_load_orders.push_back(ChunkPos{.x = cx + chunk_x, .z = cz + chunk_z});
                m_load_semaphore.release();
            }
        }

        std::lock_guard<std::mutex> lock(m_world->get_chunk_mutex());

        for (const Chunk& chunk : m_world->get_chunks())
        {
            if (chunk.x() < chunk_x - distance || chunk.x() > chunk_x + distance || chunk.z() < chunk_z - distance || chunk.z() > chunk_z + distance)
            {
                std::lock_guard<std::mutex> lock(m_unload_orders_mutex);
                if (has_unload_order(chunk.x(), chunk.z()))
                    continue;

                m_unload_orders.push_back(ChunkPos{.x = chunk.x(), .z = chunk.z()});
                m_unload_semaphore.release();
            }
        }
    }

    bool has_load_order(int64_t x, int64_t z)
    {
        for (const auto& order : m_load_orders)
        {
            if (order.x == x && order.z == z)
                return true;
        }

        return false;
    }

    bool has_unload_order(int64_t x, int64_t z)
    {
        for (const auto& order : m_unload_orders)
        {
            if (order.x == x && order.z == z)
                return true;
        }

        return false;
    }

    static void load_worker(WorldGenerator *gen)
    {
        tracy::SetThreadName("Load");

        size_t remaining = 0;

        while (gen->m_load_state.load())
        {
            if (remaining == 0)
                gen->m_load_semaphore.acquire();

            ZoneScopedN("load chunk");

            std::optional<size_t> buffer_index = gen->m_world->acquire_buffer();
            if (!buffer_index.has_value())
                continue;

            gen->m_load_orders_mutex.lock();
            std::optional<ChunkPos> chunk_pos = gen->pop_nearest_load_order(0, 0);
            remaining = gen->m_load_orders.size();
            gen->m_load_orders_mutex.unlock();

            if (chunk_pos.has_value())
            {
                if (gen->m_world->is_chunk_loaded(chunk_pos->x, chunk_pos->z))
                {
                    gen->m_world->free_buffer(*buffer_index);
                    continue;
                }

                Chunk chunk = gen->generate_chunk(chunk_pos->x, chunk_pos->z);
                chunk.set_buffer_id(*buffer_index);
                chunk.update_instance_buffer(gen->m_world->get_buffer(chunk.get_buffer_id()));

                std::lock_guard<std::mutex> lock(gen->m_world->get_chunk_mutex());
                gen->m_world->get_chunks().push_back(chunk);
            }
        }
    }

    static void unload_worker(WorldGenerator *gen)
    {
        tracy::SetThreadName("Unload");

        size_t remaining = 0;

        while (gen->m_unload_state.load())
        {
            if (remaining == 0)
                gen->m_unload_semaphore.acquire();

            ZoneScopedN("unload chunk");

            gen->m_unload_orders_mutex.lock();
            std::optional<ChunkPos> chunk_pos = gen->pop_nearest_unload_order(0, 0);
            remaining = gen->m_unload_orders.size();
            gen->m_unload_orders_mutex.unlock();

            if (chunk_pos.has_value())
            {
                std::lock_guard<std::mutex> lock(gen->m_world->get_chunk_mutex());
                std::optional<Chunk *> chunk_opt = gen->m_world->get_chunk(chunk_pos->x, chunk_pos->z);

                if (chunk_opt.has_value())
                {
                    Chunk *chunk = chunk_opt.value();

                    gen->m_world->free_buffer(chunk->get_buffer_id());
                    gen->m_world->remove_chunk(chunk->x(), chunk->z());
                }
            }
        }
    }

    static size_t get_nearest(const std::vector<ChunkPos>& chunks, int64_t x, int64_t z)
    {
        size_t min = std::numeric_limits<size_t>::max();
        size_t min_index = std::numeric_limits<size_t>::max();

        for (size_t index = 0; index < chunks.size(); index++)
        {
            const auto& chunk = chunks[index];
            size_t distance = (chunk.x - x) * (chunk.x - x) + (chunk.z - z) * (chunk.z - z);

            if (distance < min)
            {
                min = distance;
                min_index = index;
            }
        }

        return min_index;
    }

    std::optional<ChunkPos> pop_nearest_load_order(int64_t x, int64_t z)
    {
        ZoneScoped;

        size_t index = get_nearest(m_load_orders, x, z);
        if (index == std::numeric_limits<size_t>::max())
            return std::nullopt;

        ChunkPos pos = m_load_orders[index];
        m_load_orders.erase(m_load_orders.begin() + (ssize_t)index);

        return pos;
    }

    std::optional<ChunkPos> pop_nearest_unload_order(int64_t x, int64_t z)
    {
        ZoneScoped;

        size_t index = get_nearest(m_unload_orders, x, z);
        if (index == std::numeric_limits<size_t>::max())
            return std::nullopt;

        ChunkPos pos = m_unload_orders[index];
        m_unload_orders.erase(m_unload_orders.begin() + (ssize_t)index);

        return pos;
    }

    Chunk generate_chunk(int64_t chunk_x, int64_t chunk_z)
    {
        Chunk chunk(chunk_x, chunk_z);

        int64_t block_x = chunk_x * 16;
        int64_t block_z = chunk_z * 16;

        for (int64_t x = 0; x < 16; x++)
        {
            for (int64_t y = 0; y < 256; y++)
            {
                for (int64_t z = 0; z < 16; z++)
                {
                    if (!Terrain::has_block(x + block_x, y, z + block_z))
                        continue;

                    chunk.set_block(x, y, z, BlockState(1));
                }
            }
        }

        return chunk;
    }

    // private:
    Ref<World> m_world;
    std::mutex m_chunks_mutex;

    std::thread m_load_worker;
    std::atomic_bool m_load_state = true;
    std::binary_semaphore m_load_semaphore;
    std::vector<ChunkPos> m_load_orders;
    std::mutex m_load_orders_mutex;

    std::thread m_unload_worker;
    std::atomic_bool m_unload_state = true;
    std::binary_semaphore m_unload_semaphore;
    std::vector<ChunkPos> m_unload_orders;
    std::mutex m_unload_orders_mutex;
};
