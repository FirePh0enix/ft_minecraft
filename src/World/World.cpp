#include "World.hpp"

World::World(Ref<Mesh> mesh, Ref<Material> material)
    : m_mesh(mesh), m_material(material)
{
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

    for (ssize_t x = -(ssize_t)m_distance; x <= (ssize_t)m_distance; x++)
    {
        for (ssize_t z = -(ssize_t)m_distance; z <= (ssize_t)m_distance; z++)
        {
            Chunk chunk = Chunk::flat(x, z, state, 10);
            chunk.set_buffer_id(id);
            chunk.update_instance_buffer(m_buffers[id]);
            m_chunks.push_back(chunk);
            id++;
        }
    }
}

void World::encode_draw_calls(RenderGraph& graph, Camera& camera) const
{
    ZoneScoped;

    for (const Chunk& chunk : m_chunks)
    {
        Ref<Buffer> buffer = m_buffers[chunk.get_buffer_id()];
        graph.add_draw(m_mesh.ptr(), m_material.ptr(), camera.get_view_proj_matrix(), chunk.get_block_count(), buffer.ptr());
    }
}
