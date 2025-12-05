#include "World/Generator.hpp"
#include "World/Pass/Surface.hpp"
#include "World/Registry.hpp"

#include <algorithm>

Generator::Generator(const Ref<World>& world, const Ref<Shader>& shader)
    : m_world(world)
{
    m_passes.push_back(newobj(SurfacePass, world->seed()));

    m_load_thread = std::thread(load_thread, this);

    m_chunk_buffers.resize(m_max_chunk_count);
    for (ChunkBuffers& buffers : m_chunk_buffers)
    {
        buffers.block_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), Chunk::block_count, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value_or(nullptr);
        buffers.visibility_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(uint32_t), Chunk::block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value_or(nullptr);

        buffers.material = Material::create(shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);
        buffers.material->set_param("blocks", buffers.block_buffer);
        buffers.material->set_param("visibilityBuffer", buffers.visibility_buffer);
        buffers.material->set_param("positions", world->get_position_buffer());
        buffers.material->set_param("textureRegistry", BlockRegistry::get_texture_buffer());
        buffers.material->set_param("images", BlockRegistry::get_texture_array());
    }
}

void Generator::request_load(int64_t x, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world_mutex);
        if (m_world->is_chunk_loaded(x, z))
            return;
    }

    std::lock_guard<std::mutex> guard(m_load_orders_lock);
    ChunkPos pos(x, z);
    if (std::count(m_load_orders.begin(), m_load_orders.end(), pos) == 0 || std::count(m_load_orders_processing.begin(), m_load_orders_processing.end(), pos) == 0)
        m_load_orders.push_back(ChunkPos{.x = x, .z = z});
}

void Generator::request_unload(int64_t x, int64_t z)
{
    {
        std::lock_guard<std::mutex> guard(m_world_mutex);
        if (!m_world->is_chunk_loaded(x, z))
            return;
    }

    std::lock_guard<std::mutex> guard(m_unload_orders_lock);
    m_unload_orders.push_back(ChunkPos{.x = x, .z = z});
}

void Generator::load_around(int64_t x, int64_t y, int64_t z)
{
    (void)x;
    (void)y;
    (void)z;

    for (int64_t cx = -1; cx <= 2; cx++)
        for (int64_t cz = -1; cz <= 2; cz++)
            request_load(cx, cz);

    m_load_orders_semaphore.release();
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

void Generator::load_thread(Generator *g)
{
    size_t remaining = 0;

    while (g->m_load_orders_state.load())
    {
        if (remaining == 0)
            g->m_load_orders_semaphore.acquire();

        // TODO: Add back pop_nearest_order
        g->m_load_orders_lock.lock();
        ChunkPos pos = g->m_load_orders[g->m_load_orders.size() - 1];
        g->m_load_orders.pop_back();
        // g->m_load_orders_processing.push_back(pos); TODO
        remaining = g->m_load_orders.size();
        g->m_load_orders_lock.unlock();

        // TODO: Also load the chunk if its saved to disk.
        // TODO: Add some kind of memory budget to keep some chunks in memory to not have
        //       to save/read to disk everytime.

        Ref<Chunk> chunk = g->generate_chunk(pos.x, pos.z);
        g->m_world_mutex.lock();
        g->m_world->add_chunk(pos.x, pos.z, chunk);
        g->m_world_mutex.unlock();

        size_t buffers_index = g->acquire_buffers();
        ASSERT(buffers_index < std::numeric_limits<size_t>::max(), "No more buffer available");
        chunk->set_buffers(g->m_chunk_buffers[buffers_index].block_buffer, g->m_chunk_buffers[buffers_index].visibility_buffer, g->m_chunk_buffers[buffers_index].material);
    }
}

size_t Generator::acquire_buffers()
{
    std::lock_guard<std::mutex> guard(m_chunk_buffers_mutex);
    for (size_t i = 0; i < m_chunk_buffers.size(); i++)
        if (!m_chunk_buffers[i].used)
        {
            m_chunk_buffers[i].used = true;
            return i;
        }
    return std::numeric_limits<size_t>::max();
}
