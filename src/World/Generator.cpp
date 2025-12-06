#include "World/Generator.hpp"
#include "World/Chunk.hpp"
#include "World/Pass/Surface.hpp"
#include "World/Registry.hpp"

#include <algorithm>

Generator::Generator(const Ref<World>& world, const Ref<Shader>& shader)
    : m_world(world), m_visual_shader(shader)
{
    m_passes.push_back(newobj(SurfacePass, world->seed()));

    m_load_thread_pool_size = 3;
    m_load_thread_pool = new LoadThread[m_load_thread_pool_size];
    for (size_t i = 0; i < m_load_thread_pool_size; i++)
        m_load_thread_pool[i].thread = std::thread(load_thread, this, &m_load_thread_pool[i]);

    m_unload_thread = std::thread(unload_thread, this);
}

Generator::~Generator()
{
    m_load_state = false;
    for (size_t i = 0; i < m_load_thread_pool_size; i++)
    {
        // TODO: Cleanly join the threads
        // m_load_thread_pool[i].sleep_sempahore.release();
        // m_load_thread_pool[i].thread.join();
    }

    m_unload_orders_state = false;
    m_unload_orders_semaphore.release();
    m_unload_thread.join();
}

void Generator::request_load(int64_t x, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world->get_chunk_mutex());
        if (m_world->is_chunk_loaded(x, z))
            return;
    }

    ChunkPos pos{.x = x, .z = z};

    {
        std::lock_guard<std::mutex> guard(m_load_processing_mutex);
        if (std::count(m_load_orders_processing.begin(), m_load_orders_processing.end(), pos) > 0)
            return;
    }

    size_t lowest_workload = 0;
    size_t lowest_workload_index = 0;

    for (size_t thread_index = 0; thread_index < m_load_thread_pool_size; thread_index++)
    {
        if (m_load_thread_pool[thread_index].orders_count < lowest_workload)
        {
            lowest_workload = m_load_thread_pool[thread_index].orders_count;
            lowest_workload_index = thread_index;
        }
    }

    {
        std::lock_guard<std::mutex> guard(m_load_thread_pool[lowest_workload_index].mutex);
        if (std::count(m_load_thread_pool[lowest_workload_index].orders.begin(), m_load_thread_pool[lowest_workload_index].orders.end(), pos) == 0)
        {
            m_load_thread_pool[lowest_workload_index].orders.push_back(pos);
            m_load_thread_pool[lowest_workload_index].orders_count = m_load_thread_pool[lowest_workload_index].orders.size();

            {
                std::lock_guard<std::mutex> guard(m_load_processing_mutex);
                m_load_orders_processing.push_back(pos);
            }

            m_load_thread_pool[lowest_workload_index].sleep_sempahore.release();
        }
    }
}

void Generator::request_unload(int64_t x, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world->get_chunk_mutex());
        if (!m_world->is_chunk_loaded(x, z))
            return;
    }

    ChunkPos pos{.x = x, .z = z};

    std::lock_guard<std::mutex> guard(m_unload_orders_lock);
    if (std::count(m_unload_orders.begin(), m_unload_orders.end(), pos) == 0)
        m_unload_orders.push_back(ChunkPos{.x = x, .z = z});
}

void Generator::set_distance(uint32_t distance)
{
    m_load_distance = distance;
    m_max_chunk_count = ((m_load_distance * 2) + 1) * ((m_load_distance * 2) + 1);

    // m_chunk_buffers = new ChunkBuffers[m_max_chunk_count];
    // for (size_t index = 0; index < m_max_chunk_count; index++)
    // {
    //     ChunkBuffers& buffers = m_chunk_buffers[index];
    //     buffers.block_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), Chunk::block_count, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value_or(nullptr);
    //     buffers.visibility_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(uint32_t), Chunk::block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value_or(nullptr);

    //     buffers.material = Material::create(m_visual_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);
    //     buffers.material->set_param("blocks", buffers.block_buffer);
    //     buffers.material->set_param("visibilityBuffer", buffers.visibility_buffer);
    //     buffers.material->set_param("positions", m_world->get_position_buffer());
    //     buffers.material->set_param("textureRegistry", BlockRegistry::get_texture_buffer());
    //     buffers.material->set_param("images", BlockRegistry::get_texture_array());
    // }
}

void Generator::load_around(int64_t x, int64_t y, int64_t z)
{
    (void)y;
    int64_t middle_x = (int64_t)glm::round((double)x / 16.0);
    int64_t middle_z = (int64_t)glm::round((double)z / 16.0);

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++)
        for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++)
            request_load(cx + middle_x, cz + middle_z);

    for (const auto& [pos, chunk] : m_world->get_dimension(0))
    {
        if (pos.x < middle_x - m_load_distance || pos.x > middle_x + m_load_distance || pos.z < middle_z - m_load_distance || pos.z > middle_z + m_load_distance)
            request_unload(pos.x, pos.z);
    }

    m_unload_orders_semaphore.release();
}

Ref<Chunk> Generator::generate_chunk(int64_t cx, int64_t cz)
{
    Ref<Chunk> chunk = newobj(Chunk, cx, cz);

    std::vector<BlockState> blocks(Chunk::block_count);

    for (size_t index = 0; index < m_passes.size(); index++)
    {
        Ref<GeneratorPass>& pass = m_passes[index];
        for (int64_t x = 0; x < 16; x++)
        {
            for (int64_t z = 0; z < 16; z++)
            {
                for (int64_t y = 0; y < 256; y++)
                {
                    int64_t gx = x + cx * 16;
                    int64_t gz = z + cz * 16;

                    BlockState block = pass->process(blocks, gx, y, gz);
                    blocks[z * 16 * 256 + y * 16 + x] = block;
                }
            }
        }
    }

    chunk->set_blocks(blocks);

    return chunk;
}

void Generator::load_thread(Generator *g, LoadThread *t)
{
    size_t remaining = 0;

    while (g->m_load_state.load())
    {
        if (remaining == 0)
            t->sleep_sempahore.acquire();

        // size_t buffers_index = g->acquire_buffers();
        // if (buffers_index == std::numeric_limits<size_t>::max())
        // {
        //     // There is no more available buffers for us to render a new chunk, so the best way to handle it is to wait
        //     // until the error resolve itself when chunk will be freed.
        //     // This error should not happen in normal circonstances but there is no need to crash on it.
        //     continue;
        // }

        // TODO: Add back pop_nearest_order
        ChunkPos pos{};
        {
            std::lock_guard<std::mutex> guard(t->mutex);

            if (t->orders.size() == 0)
            {
                // g->release_buffers(buffers_index);
                continue;
            }

            // pos = t->orders[t->orders.size() - 1];

            // t->orders.pop_back();
            // remaining = t->orders.size();
            // t->orders_count = t->orders.size();
            pos = t->pop_nearest_chunk(g->m_reference_position);
        }

        // TODO: Also load the chunk if its saved to disk.
        // TODO: Add some kind of memory budget to keep some chunks in memory to not have
        //       to save/read to disk everytime.

        Ref<Chunk> chunk = g->generate_chunk(pos.x, pos.z);
        chunk->set_buffers(g->m_visual_shader, g->m_world->get_position_buffer());

        {
            std::lock_guard<std::mutex> guard(g->m_world->get_chunk_mutex());
            g->m_world->add_chunk(pos.x, pos.z, chunk);
        }
    }
}

void Generator::unload_thread(Generator *g)
{
    size_t remaining = 0;

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
            std::lock_guard<std::mutex> guard(g->m_world->get_chunk_mutex());
            // g->release_buffers(g->m_world->get_chunk(pos.x, pos.z)->ptr()->get_buffers_index());
            g->m_world->remove_chunk(pos.x, pos.z);
        }
    }
}

// size_t Generator::acquire_buffers()
// {
//     std::lock_guard<std::mutex> guard(m_chunk_buffers_mutex);
//     for (size_t i = 0; i < m_max_chunk_count; i++)
//         if (!m_chunk_buffers[i].used)
//         {
//             m_chunk_buffers[i].used = true;
//             return i;
//         }
//     return std::numeric_limits<size_t>::max();
// }

// void Generator::release_buffers(size_t index)
// {
//     std::lock_guard<std::mutex> guard(m_chunk_buffers_mutex);
//     m_chunk_buffers[index].used = false;
// }

ChunkPos Generator::LoadThread::pop_nearest_chunk(glm::vec3 position)
{
    ChunkPos chunk_pos = orders[0];
    size_t index = 0;
    float lowest_distance_sq = INFINITY;

    for (size_t i = 0; i < orders.size(); i++)
    {
        const ChunkPos& pos = orders[i];
        float distance_sq = glm::distance2(position, glm::vec3(pos.x * 16.0 + 8.0, 128.0, pos.z * 16.0 + 8.0));

        if (distance_sq < lowest_distance_sq)
        {
            lowest_distance_sq = distance_sq;
            chunk_pos = pos;
            index = i;
        }
    }

    orders.erase(orders.begin() + index);

    return chunk_pos;
}
