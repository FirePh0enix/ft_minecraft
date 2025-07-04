#pragma once

#include "Core/Print.hpp"
#include "World/Generation/Terrain.hpp"
#include "World/Registry.hpp"
#include "World/Save.hpp"
#include "World/World.hpp"

#include <mutex>
#include <semaphore>
#include <thread>

struct WorldGenerationSettings
{
    uint64_t seed;
};

class WorldGenerator : public Object
{
    CLASS(WorldGenerator, Object);

public:
    WorldGenerator(Ref<World> world, bool disable_save = false)
        : m_world(world), player_position(0.0), save("new_world"), disable_save(disable_save), m_load_semaphore(0), m_unload_semaphore(0)
    {
        m_load_worker = std::thread(load_worker, this);
        m_unload_worker = std::thread(unload_worker, this);

        save.save_info(world);
    }

    ~WorldGenerator()
    {
    }

    void stop_workers()
    {
        m_load_state.store(false);
        m_unload_state.store(false);

        m_load_semaphore.release();
        m_unload_semaphore.release();

        m_load_worker.join();
        m_unload_worker.join();
    }

    inline void set_terrain(Ref<TerrainGenerator> terrain)
    {
        m_terrain = terrain;
    }

    inline void set_player_pos(glm::vec3 pos)
    {
        player_position = pos;
    }

    void load_around(int64_t x, int64_t y, int64_t z)
    {
        ZoneScoped;

        (void)y;

        const int64_t chunk_x = (int64_t)((float)x / 16.0f);
        const int64_t chunk_z = (int64_t)((float)z / 16.0f);
        const int64_t distance = m_world->get_distance();

        {
            ZoneScopedN("load_around.load");
            std::lock_guard<std::mutex> lock(m_load_orders_mutex);

            for (int64_t cx = -distance; cx <= distance; cx++)
            {
                for (int64_t cz = -distance; cz <= distance; cz++)
                {
                    if (has_load_order(cx + chunk_x, cz + chunk_z))
                        continue;
                    m_load_orders.push_back(ChunkPos{.x = cx + chunk_x, .z = cz + chunk_z});
                }
            }
        }
        if (m_load_orders.size() > 0)
            m_load_semaphore.release();

        std::lock_guard<std::mutex> lock(m_world->get_chunk_read_mutex());

        {
            ZoneScopedN("load_around.unload");

            for (const auto& chunk : m_world->get_dimension(0))
            {
                if (chunk.first.x < chunk_x - distance || chunk.first.x > chunk_x + distance || chunk.first.z < chunk_z - distance || chunk.first.z > chunk_z + distance)
                {
                    std::lock_guard<std::mutex> lock(m_unload_orders_mutex);
                    if (has_unload_order(chunk.first.x, chunk.first.z))
                        continue;

                    m_unload_orders.push_back(ChunkPos{.x = chunk.first.x, .z = chunk.first.z});
                }
            }
        }

        if (m_unload_orders.size() > 0)
            m_unload_semaphore.release();
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

            int64_t pcx = (int64_t)gen->player_position.x / 16;
            int64_t pcz = (int64_t)gen->player_position.z / 16;

            gen->m_load_orders_mutex.lock();
            std::optional<ChunkPos> chunk_pos_opt = gen->pop_nearest_load_order(pcx, pcz);
            remaining = gen->m_load_orders.size();
            gen->m_load_orders_mutex.unlock();

            if (chunk_pos_opt.has_value())
            {
                ChunkPos chunk_pos = chunk_pos_opt.value();

                if (gen->m_world->is_chunk_loaded(chunk_pos.x, chunk_pos.z))
                {
                    gen->m_world->free_buffer(*buffer_index);
                    continue;
                }

                Ref<Chunk> chunk;

                if (!gen->disable_save && gen->save.chunk_exists(chunk_pos.x, chunk_pos.z))
                {
                    chunk = gen->save.load_chunk(chunk_pos.x, chunk_pos.z);
                }
                else
                {
                    chunk = gen->generate_chunk(chunk_pos.x, chunk_pos.z);

                    if (!gen->disable_save)
                        gen->save.save_chunk(chunk);
                }

                chunk->set_buffer_id(buffer_index.value());

                {
                    std::lock_guard<std::mutex> lock(gen->m_world->get_chunk_read_mutex());
                    chunk->compute_full_visibility(gen->m_world);
                }

                chunk->update_instance_buffer(gen->m_world->get_buffer(chunk->get_buffer_id()));

                {
                    ZoneScopedN("update neighbour chunks");

                    {
                        std::lock_guard<std::mutex> lock(gen->m_world->get_chunk_add_mutex());
                        gen->m_world->add_chunk(chunk->x(), chunk->z(), chunk);
                    }

                    std::lock_guard<std::mutex> lock(gen->m_world->get_chunk_read_mutex());

                    for (auto& chunk_maybe : {
                             gen->m_world->get_chunk(chunk_pos.x, chunk_pos.z - 1),
                             gen->m_world->get_chunk(chunk_pos.x, chunk_pos.z + 1),
                             gen->m_world->get_chunk(chunk_pos.x - 1, chunk_pos.z),
                             gen->m_world->get_chunk(chunk_pos.x + 1, chunk_pos.z),
                         })
                    {
                        if (chunk_maybe.has_value())
                        {
                            Ref<Chunk> chunk2 = chunk_maybe.value();
                            chunk2->compute_axis_neighbour_visibility(gen->m_world, chunk);
                            chunk2->update_instance_buffer(gen->m_world->get_buffer(chunk2->get_buffer_id()));
                        }
                    }
                }
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
            std::optional<ChunkPos> chunk_pos = gen->pop_unload_order();
            remaining = gen->m_unload_orders.size();
            gen->m_unload_orders_mutex.unlock();

            if (chunk_pos.has_value())
            {
                std::lock_guard<std::mutex> lock_add(gen->m_world->get_chunk_add_mutex());
                std::lock_guard<std::mutex> lock_read(gen->m_world->get_chunk_read_mutex());
                std::optional<Ref<Chunk>> chunk_opt = gen->m_world->get_chunk(chunk_pos->x, chunk_pos->z);

                if (chunk_opt.has_value())
                {
                    Ref<Chunk> chunk = chunk_opt.value();

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

    std::optional<ChunkPos> pop_unload_order()
    {
        ZoneScoped;

        if (m_unload_orders.empty())
            return std::nullopt;

        ChunkPos pos = m_unload_orders.back();
        m_unload_orders.pop_back();

        return pos;
    }

    Ref<Chunk> generate_chunk(int64_t chunk_x, int64_t chunk_z)
    {
        ZoneScoped;

        Ref<Chunk> chunk = make_ref<Chunk>(chunk_x, chunk_z);

        int64_t block_x = chunk_x * 16;
        int64_t block_z = chunk_z * 16;

        for (int64_t x = 0; x < 16; x++)
        {
            for (int64_t y = 0; y < 256; y++)
            {
                for (int64_t z = 0; z < 16; z++)
                {
                    if (!m_terrain->has_block(x + block_x, y, z + block_z))
                        continue;

                    auto id = BlockRegistry::get().get_block_id("stone");

                    BiomeNoise noise{
                        .continentalness = m_terrain->get_continentalness_noise(x + block_x, z + block_z),
                        .erosion = m_terrain->get_erosion_noise(x + block_x, z + block_z),
                        .peaks_and_valleys = m_terrain->get_peaks_and_valleys_noise(x + block_x, z + block_z),
                        .temperature_noise = m_terrain->get_temperature_noise(x + block_x, z + block_z),
                        .humidity_noise = m_terrain->get_humidity_noise(x + block_x, z + block_z),
                    };

                    Biome biome = m_terrain->get_biome(noise);

                    if (biome == Biome::Ocean || biome == Biome::River)
                    {
                        id = BlockRegistry::get().get_block_id("water");
                    }

                    else if (biome == Biome::FrozenRiver)
                    {
                        id = BlockRegistry::get().get_block_id("snow");
                    }
                    else if (biome == Biome::StonyShore || biome == Biome::StonyPeaks)
                    {
                        id = BlockRegistry::get().get_block_id("stone");
                    }

                    else if (biome == Biome::FrozenPeaks || biome == Biome::SnowyPlains)
                    {
                        id = BlockRegistry::get().get_block_id("snow");
                    }

                    else if (biome == Biome::Beach || biome == Biome::Desert)
                    {
                        id = BlockRegistry::get().get_block_id("sand");
                    }

                    else if (biome == Biome::Taiga || biome == Biome::Plains || biome == Biome::Savanna)
                    {
                        id = BlockRegistry::get().get_block_id("grass");
                    }

                    chunk.set_block(x, y, z, BlockState(id));
                }
            }
        }

        return chunk;
    }

    // private:
    Ref<World> m_world;
    glm::vec3 player_position;

    // generators
    Ref<TerrainGenerator> m_terrain;

    Save save;
    bool disable_save;

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
