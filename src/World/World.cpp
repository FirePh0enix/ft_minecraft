#include "World.hpp"

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
    const size_t chunk_local_x = x > 0 ? (x % 16) : -(x % 16);
    const size_t chunk_local_z = z > 0 ? (z % 16) : -(z % 16);

    return chunk->get_block(chunk_local_x, y, chunk_local_z);
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
    const size_t chunk_local_x = x > 0 ? (x % 16) : -(x % 16);
    const size_t chunk_local_z = z > 0 ? (z % 16) : -(z % 16);

    chunk->set_block(chunk_local_x, y, chunk_local_z, state);
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

    m_distance = distance;

    // Create or recreate instance buffers.
    m_buffers.resize(new_buffer_count);

    if (new_buffer_count <= old_buffer_count)
        return;

    for (size_t i = 0; i < new_buffer_count - old_buffer_count; i++)
    {
        auto buffer_result = RenderingDriver::get()->create_buffer(sizeof(BlockInstanceData) * Chunk::block_count, {.copy_dst = true, .vertex = true});
        ERR_EXPECT_C(buffer_result, "Failed to create instance buffer");
        m_buffers[i + old_buffer_count] = buffer_result.value();
    }
}

void World::generate_flat(BlockState state)
{
    size_t id = 0;

    for (ssize_t x = -m_distance; x <= m_distance; x++)
    {
        for (ssize_t z = -m_distance; z <= m_distance; z++)
        {
            Chunk chunk = Chunk::flat(x, z, state, 10);
            chunk.set_buffer_id(id);
            chunk.update_instance_buffer(m_buffers[id]);
            m_dims[0].get_chunks().push_back(chunk);
            id++;
        }
    }
    std::println("{} {}", -m_distance, +m_distance);
}

void World::update_buffers()
{
    for (auto& chunk : m_dims[0].get_chunks())
    {
        chunk.update_instance_buffer(m_buffers[chunk.get_buffer_id()]);
    }
}

void World::encode_draw_calls(RenderGraph& graph, Camera& camera) const
{
    ZoneScoped;

    for (const Chunk& chunk : m_dims[0].get_chunks())
    {
        Ref<Buffer> buffer = m_buffers[chunk.get_buffer_id()];
        graph.add_draw(m_mesh.ptr(), m_material.ptr(), camera.get_view_proj_matrix(), chunk.get_block_count(), buffer.ptr());
    }
}
