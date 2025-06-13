#include "World.hpp"
#include "Core/Format.hpp"
#include "Core/Logger.hpp"

World::World(Ref<Mesh> mesh, Ref<Material> material)
    : m_mesh(mesh), m_material(material)
{
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x / 16;
    int64_t chunk_z = z / 16;

    std::optional<const Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return BlockState();
    }

    const Chunk *chunk = chunk_value.value();
    return chunk->get_block(x - chunk_x * 16, y, z - chunk_z * 16);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    int64_t chunk_x = x / 16;
    int64_t chunk_z = z / 16;

    std::optional<Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Chunk *chunk = chunk_value.value();
    chunk->set_block(x - chunk_x * 16, y, z - chunk_z * 16, state);
}

std::optional<const Chunk *> World::get_chunk(int64_t x, int64_t z) const
{
    for (const Chunk& chunk : m_dims[0].get_chunks())
    {
        if (chunk.x() == x && chunk.z() == z)
            return &chunk;
    }
    return std::nullopt;
}

std::optional<Chunk *> World::get_chunk(int64_t x, int64_t z)
{
    for (Chunk& chunk : m_dims[0].get_chunks())
    {
        if (chunk.x() == x && chunk.z() == z)
            return &chunk;
    }
    return std::nullopt;
}

void World::set_render_distance(uint32_t distance)
{
    const uint32_t old_buffer_count = m_buffers.size();
    const uint32_t new_buffer_count = (distance * 2 + 1) * (distance * 2 + 1);

    std::lock_guard<std::mutex> lock(m_buffers_mutex);

    m_distance = distance;

    // Create or recreate instance buffers.
    m_buffers.resize(new_buffer_count);

    if (new_buffer_count <= old_buffer_count)
        return;

    for (size_t i = 0; i < new_buffer_count - old_buffer_count; i++)
    {
        auto buffer_result = RenderingDriver::get()->create_buffer(sizeof(BlockInstanceData) * Chunk::block_count, {.copy_dst = true, .vertex = true});
        ERR_EXPECT_C(buffer_result, "Failed to create instance buffer");
        m_buffers[i + old_buffer_count] = BufferInfo{.buffer = buffer_result.value(), .used = false};
    }

    info("{} bytes of VRAM allocated for chunk buffers", FormatBin(m_buffers.size() * sizeof(BlockInstanceData) * Chunk::block_count));
}

void World::encode_draw_calls(RenderGraph& graph, Camera& camera)
{
    ZoneScoped;

    std::lock_guard<std::mutex> lock(m_chunks_mutex);

    for (const Chunk& chunk : m_dims[0].get_chunks())
    {
        const Ref<Buffer>& buffer = m_buffers[chunk.get_buffer_id()].buffer;
        graph.add_draw(m_mesh.ptr(), m_material.ptr(), camera.get_view_proj_matrix(), chunk.get_block_count(), buffer.ptr());
    }
}

std::optional<size_t> World::acquire_buffer()
{
    std::lock_guard<std::mutex> lock(m_buffers_mutex);

    for (size_t i = 0; i < m_buffers.size(); i++)
    {
        auto& info = m_buffers[i];

        if (!info.used)
        {
            info.used = true;
            return i;
        }
    }

    return std::nullopt;
}

void World::free_buffer(size_t index)
{
    std::lock_guard<std::mutex> lock(m_buffers_mutex);
    m_buffers[index].used = false;
}
