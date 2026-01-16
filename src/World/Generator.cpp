#include "World/Generator.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Pass/Surface.hpp"
#include "World/World.hpp"

#include <algorithm>

Generator::Generator(World *world, size_t dimension)
    : m_world(world), m_dimension(dimension)
{
    m_passes.push_back(newobj(SurfacePass));

    m_load_thread = std::thread(load_thread, this);
    m_unload_thread = std::thread(unload_thread, this);
}

Generator::~Generator()
{
    m_load_state = false;
    m_load_orders_semaphore.release();
    m_load_thread.join();

    m_unload_orders_state = false;
    m_unload_orders_semaphore.release();
    m_unload_thread.join();
}

void Generator::add_pass(Ref<GeneratorPass> pass)
{
    pass->set_seed(m_world->seed());
    m_passes.push_back(pass);
}

void Generator::request_load(int64_t x, int64_t y, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
        if (m_world->is_chunk_loaded(x, y, z))
            return;
    }

    ChunkPos pos(x, y, z);

    std::lock_guard<std::mutex> guard(m_load_orders_lock);
    m_load_orders.insert(pos);

    m_load_orders_semaphore.release();
}

void Generator::request_multiple_load(View<ChunkPos> chunks)
{
    ZoneScoped;

    std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
    std::lock_guard<std::mutex> guard2(m_load_orders_lock);
    for (const ChunkPos& pos : chunks)
    {
        if (m_world->is_chunk_loaded(pos.x, pos.y, pos.z))
            return;

        m_load_orders.insert(pos);
    }

    m_load_orders_semaphore.release();
}

void Generator::request_unload(int64_t x, int64_t y, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
        if (!m_world->is_chunk_loaded(x, y, z))
            return;
    }

    ChunkPos pos(x, y, z);

    std::lock_guard<std::mutex> guard(m_unload_orders_lock);
    m_unload_orders.push_back(pos);

    m_unload_orders_semaphore.release();
}

void Generator::set_distance(uint32_t distance)
{
    m_load_distance = distance;
    m_max_chunk_count = ((m_load_distance * 2) + 1) * ((m_load_distance * 2) + 1);
}

void Generator::load_around(int64_t x, int64_t y, int64_t z)
{
    ZoneScopedN("Generator::load_around");

    int64_t middle_x = (int64_t)glm::round((double)x / 16.0);
    int64_t middle_y = (int64_t)glm::round((double)y / 16.0);
    int64_t middle_z = (int64_t)glm::round((double)z / 16.0);

    std::vector<ChunkPos> load_positions;

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++)
        for (int64_t cy = -m_load_distance; cy <= m_load_distance; cy++)
            for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++)
            {
                load_positions.push_back(ChunkPos(cx + middle_x, cy + middle_y, cz + middle_z));
            }

    request_multiple_load(load_positions);

    std::lock_guard<std::mutex> guard(m_load_orders_lock);
    for (const auto& [pos, chunk] : m_world->get_dimension(0).get_chunks())
    {
        if (pos.x < middle_x - m_load_distance || pos.x > middle_x + m_load_distance || pos.y < middle_y - m_load_distance || pos.y > middle_y + m_load_distance || pos.z < middle_z - m_load_distance || pos.z > middle_z + m_load_distance)
        {
            request_unload(pos.x, pos.y, pos.z);
            m_load_orders.erase(pos);
        }
    }
}

Ref<Chunk> Generator::generate_chunk(int64_t cx, int64_t cy, int64_t cz)
{
    ZoneScoped;

    Ref<Chunk> chunk = newobj(Chunk, cx, cy, cz);
    if (!chunk)
        return chunk;

    for (size_t index = 0; index < m_passes.size(); index++)
    {
        Ref<GeneratorPass>& pass = m_passes[index];
        for (int64_t x = 0; x < 18; x++)
        {
            for (int64_t z = 0; z < 18; z++)
            {
                int64_t gx = cx * 16 + (x - 1);
                int64_t gy = cy * 16;
                int64_t gz = cz * 16 + (z - 1);

                pass->process(gx, gy, gz, x, z, chunk->get_blocks());
            }
        }
    }

    return chunk;
}

void Generator::load_thread(Generator *g)
{
    size_t remaining = 0;

    TracySetThreadName("Load");

    while (g->m_load_state.load())
    {
        if (remaining == 0)
            g->m_load_orders_semaphore.acquire();

        // size_t buffers_index = g->acquire_buffers();
        // if (buffers_index == std::numeric_limits<size_t>::max())
        // {
        //     // There is no more available buffers for us to render a new chunk, so the best way to handle it is to wait
        //     // until the error resolve itself when chunk will be freed.
        //     // This error should not happen in normal circonstances but there is no need to crash on it.
        //     continue;
        // }

        ChunkPos pos{};
        {
            std::lock_guard<std::mutex> guard(g->m_load_orders_lock);

            if (g->m_load_orders.size() == 0)
            {
                // g->release_buffers(buffers_index);
                continue;
            }

            pos = g->pop_nearest_chunk(g->m_reference_position);
            remaining = g->m_load_orders.size();

            int64_t middle_x = (int64_t)glm::round((double)g->m_reference_position.x / 16.0);
            int64_t middle_y = (int64_t)glm::round((double)g->m_reference_position.y / 16.0);
            int64_t middle_z = (int64_t)glm::round((double)g->m_reference_position.z / 16.0);
            if (pos.x < middle_x - g->m_load_distance || pos.x > middle_x + g->m_load_distance || pos.z < middle_z - g->m_load_distance || pos.y > middle_y + g->m_load_distance || pos.z < middle_z - g->m_load_distance || pos.z > middle_z + g->m_load_distance)
                continue;
        }

        // TODO: Also load the chunk if its saved to disk.
        // TODO: Add some kind of memory budget to keep some chunks in memory to not have
        //       to save/read to disk everytime.

        Ref<Chunk> chunk = g->generate_chunk(pos.x, pos.y, pos.z);
        chunk->build_simple_mesh();

        // println("new chunk generated: ({}, {}, {}), {} remaining", pos.x, pos.y, pos.z, g->m_load_orders.size());

        {
            std::lock_guard<std::mutex> guard(g->m_world->get_dimension(g->m_dimension).mutex());
            g->m_world->add_chunk(pos.x, pos.y, pos.z, chunk);
        }
    }
}

void Generator::unload_thread(Generator *g)
{
    size_t remaining = 0;

    TracySetThreadName("Unload");

    while (g->m_unload_orders_state.load())
    {
        if (remaining == 0)
            g->m_unload_orders_semaphore.acquire();

        // TODO: Add back pop_farsest_order
        ChunkPos pos{};
        {
            std::lock_guard<std::mutex> guard(g->m_unload_orders_lock);

            if (g->m_unload_orders.size() == 0)
            {
                // An error that should not happen, but is not an excuse to crash.
                continue;
            }

            pos = g->m_unload_orders[g->m_unload_orders.size() - 1];

            g->m_unload_orders.pop_back();
            remaining = g->m_unload_orders.size();
        }

        {
            std::lock_guard<std::mutex> guard(g->m_world->get_dimension(g->m_dimension).mutex());
            // g->release_buffers(g->m_world->get_chunk(pos.x, pos.z)->ptr()->get_buffers_index());
            g->m_world->remove_chunk(pos.x, pos.y, pos.z);
        }
    }
}

ChunkPos Generator::pop_nearest_chunk(glm::vec3 position)
{
    ChunkPos chunk_pos = *m_load_orders.begin();
    float lowest_distance_sq = INFINITY;

    for (auto& pos : m_load_orders)
    {
        float distance_sq = glm::distance2(position, glm::vec3((double)pos.x * 16.0 + 8.0, (double)pos.y * 16.0 + 8.0, (double)pos.z * 16.0 + 8.0));

        if (distance_sq < lowest_distance_sq)
        {
            lowest_distance_sq = distance_sq;
            chunk_pos = pos;
        }
    }

    m_load_orders.erase(chunk_pos);

    return chunk_pos;
}
