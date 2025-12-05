#include "World/World.hpp"
#include "Core/DataBuffer.hpp"

#include <tracy/Tracy.hpp>

World::World(Ref<Mesh> mesh, Ref<Shader> visual_shader, uint64_t seed)
    : m_seed(seed), m_mesh(mesh), m_visual_shader(visual_shader)
{
    m_surface_shader = Shader::load("assets://shaders/terrain/surface.slang", true).value_or(nullptr);
    m_visibility_shader = Shader::load("assets://shaders/terrain/visibility.slang", true).value_or(nullptr);

    m_position_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(glm::vec4), Chunk::block_count, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value();

    std::vector<glm::vec4> positions(Chunk::block_count);

    for (size_t x = 0; x < 16; x++)
        for (size_t y = 0; y < 256; y++)
            for (size_t z = 0; z < 16; z++)
                positions[x + y * 16 + z * 16 * 256] = glm::vec4(x, y, z, 0.0);

    m_position_buffer->update(View(positions).as_bytes());
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return BlockState();
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const size_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    return chunk->get_block(chunk_local_x, y, chunk_local_z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const size_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    chunk->set_block(chunk_local_x, y, chunk_local_z, state);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[0].get_chunk(x, z);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z)
{
    return m_dims[0].get_chunk(x, z);
}

void World::encode_draw_calls(RenderPassEncoder& encoder, Camera& camera)
{
    ZoneScoped;

    for (const auto& [pos, chunk] : m_dims[0])
    {
        AABB aabb = AABB(glm::vec3((float)pos.x * 16.0 + 8.0, 128.0, (float)pos.z * 16.0 + 8.0), glm::vec3(8.0, 128.0, 8.0));

        // if (!camera.frustum().contains(aabb) || !chunk->ready)
        if (!camera.frustum().contains(aabb))
            continue;

        const glm::mat4& view_matrix = camera.get_view_proj_matrix();
        const glm::mat4 model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos.x * 16, 0.0, pos.z * 16));

        DataBuffer push_constants(sizeof(glm::mat4));
        push_constants.add(view_matrix);
        push_constants.add(model_matrix);

        encoder.bind_material(chunk->get_visual_material());
        encoder.push_constants(push_constants);
        encoder.bind_index_buffer(m_mesh->get_buffer(MeshBufferKind::Index));
        encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Position), 0);
        encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Normal), 1);
        encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::UV), 2);
        encoder.draw(m_mesh->vertex_count(), Chunk::block_count);
    }
}
