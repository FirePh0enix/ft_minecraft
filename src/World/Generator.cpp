#include "World/Generator.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/World.hpp"

Generator::Generator(World *world, size_t dimension)
    : m_world(world), m_dimension(dimension)
{
    m_load_thread = std::thread(&Generator::load_thread, this);
    m_unload_thread = std::thread(&Generator::unload_thread, this);
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

void Generator::add_pass(Ref<GenerationPass> pass)
{
    pass->update_seed(m_world->seed());
    m_passes.push_back(pass);
}

void Generator::request_multiple_load(View<ChunkPos> chunks)
{
    std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
    std::lock_guard<std::mutex> guard2(m_load_orders_mutex);
    for (const ChunkPos& pos : chunks)
    {
        if (m_world->get_dimension(m_dimension).has_chunk(pos.x, pos.z))
            return;

        m_load_orders.insert(pos);
    }

    m_load_orders_semaphore.release();
}

void Generator::request_unload(int64_t x, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
        if (!m_world->get_dimension(m_dimension).has_chunk(x, z))
            return;
    }

    ChunkPos pos(x, z);

    std::lock_guard<std::mutex> guard(m_unload_orders_mutex);
    m_unload_orders.insert(pos);

    m_unload_orders_semaphore.release();
}

void Generator::set_distance(uint32_t distance)
{
    m_load_distance = distance;
    m_max_chunk_count = ((m_load_distance * 2) + 1) * ((m_load_distance * 2) + 1);
}

void Generator::load_around(int64_t x, int64_t z)
{
    int64_t middle_x = (int64_t)glm::round((double)x / 16.0);
    int64_t middle_z = (int64_t)glm::round((double)z / 16.0);

    std::vector<ChunkPos> load_positions;

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++)
        for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++)
        {
            load_positions.push_back(ChunkPos(cx + middle_x, cz + middle_z));
        }

    request_multiple_load(load_positions);

    std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
    for (const auto& [pos, chunk] : m_world->get_dimension(m_dimension).get_chunks())
    {
        // {
        //     std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
        //     if (!m_world->get_dimension(m_dimension).has_chunk(pos.x, pos.z))
        //         continue;
        // }

        if (pos.x < middle_x - m_load_distance || pos.x > middle_x + m_load_distance || pos.z < middle_z - m_load_distance || pos.z > middle_z + m_load_distance)
        {
            {
                std::lock_guard<std::mutex> guard(m_unload_orders_mutex);
                m_unload_orders.insert(pos);
            }

            {
                std::lock_guard<std::mutex> guard(m_load_orders_mutex);
                m_load_orders.erase(pos);
            }
        }
        m_unload_orders_semaphore.release();
    }
}

Ref<Chunk> Generator::generate_chunk(int64_t cx, int64_t cz)
{
    ZoneScoped;

    Ref<Chunk> chunk = newobj(Chunk, cx, cz, m_world);
    if (!chunk)
        return chunk;

    for (size_t index = 0; index < m_passes.size(); index++)
    {
        Ref<GenerationPass>& pass = m_passes[index];
        for (int64_t x = 0; x < Chunk::width_with_overlap; x++)
        {
            for (int64_t y = 0; y < Chunk::height; y++)
            {
                for (int64_t z = 0; z < Chunk::width_with_overlap; z++)
                {
                    int64_t gx = cx * 16 + (x - 1);
                    int64_t gz = cz * 16 + (z - 1);

                    size_t block_index = Chunk::linearize_with_overlap(x, y, z);

                    pass->generate(gx, y, gz, Chunk::linearize_with_overlap(x, y, z), chunk);
                    BlockState state = chunk->get_blocks()[block_index];

                    // there is at least one non-empty block.
                    if (!state.is_air())
                        chunk->get_slices()[y / Chunk::width].empty = false;
                }
            }
        }
    }

    for (size_t i = 0; i < Chunk::slice_count; i++)
        chunk->build_simple_mesh(i);

    return chunk;
}

void Generator::load_thread()
{
    size_t remaining = 0;

    TracySetThreadName("Load");

    while (m_load_state.load())
    {
        if (remaining == 0)
            m_load_orders_semaphore.acquire();

        if (!m_load_state.load())
            break;

        ChunkPos pos{};
        {
            std::lock_guard<std::mutex> guard(m_load_orders_mutex);

            if (m_load_orders.size() == 0)
            {
                continue;
            }

            pos = pop_nearest_chunk(m_reference_position);
            remaining = m_load_orders.size();

            int64_t middle_x = (int64_t)glm::round((double)m_reference_position.x / 16.0);
            int64_t middle_z = (int64_t)glm::round((double)m_reference_position.z / 16.0);
            if (pos.x < middle_x - m_load_distance || pos.x > middle_x + m_load_distance || pos.z < middle_z - m_load_distance || pos.z > middle_z + m_load_distance)
                continue;
        }

        // TODO: Also load the chunk if its saved to disk.
        // TODO: Add some kind of memory budget to keep some chunks in memory to not have
        //       to save/read to disk everytime.

        Ref<Chunk> chunk = generate_chunk(pos.x, pos.z);

        {
            std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
            m_world->get_dimension(m_dimension).add_chunk(pos.x, pos.z, chunk);
        }
    }
}

void Generator::unload_thread()
{
    size_t remaining = 0;

    TracySetThreadName("Unload");

    while (m_unload_orders_state.load())
    {
        if (remaining == 0)
            m_unload_orders_semaphore.acquire();

        if (!m_unload_orders_state.load())
            break;

        // TODO: Add back pop_farsest_order
        ChunkPos pos{};
        {
            std::lock_guard<std::mutex> guard(m_unload_orders_mutex);

            if (m_unload_orders.size() == 0)
            {
                continue;
            }

            pos = pop_farsest_chunk(m_reference_position);
            remaining = m_unload_orders.size();
        }

        {
            std::lock_guard<std::mutex> guard(m_world->get_dimension(m_dimension).mutex());
            m_world->get_dimension(m_dimension).remove_chunk(pos.x, pos.z);
        }
    }
}

ChunkPos Generator::pop_nearest_chunk(glm::vec3 position)
{
    ChunkPos chunk_pos = *m_load_orders.begin();
    float lowest_distance_sq = INFINITY;

    for (auto& pos : m_load_orders)
    {
        float distance_sq = glm::distance2(position, glm::vec3((double)pos.x * 16.0 + 8.0, 0, (double)pos.z * 16.0 + 8.0));

        if (distance_sq < lowest_distance_sq)
        {
            lowest_distance_sq = distance_sq;
            chunk_pos = pos;
        }
    }

    m_load_orders.erase(chunk_pos);

    return chunk_pos;
}

ChunkPos Generator::pop_farsest_chunk(glm::vec3 position)
{
    ChunkPos chunk_pos = *m_unload_orders.begin();
    float lowest_distance_sq = INFINITY;

    for (auto& pos : m_unload_orders)
    {
        float distance_sq = glm::distance2(position, glm::vec3((double)pos.x * 16.0 + 8.0, 0, (double)pos.z * 16.0 + 8.0));

        if (distance_sq < lowest_distance_sq)
        {
            lowest_distance_sq = distance_sq;
            chunk_pos = pos;
        }
    }

    m_unload_orders.erase(chunk_pos);

    return chunk_pos;
}
