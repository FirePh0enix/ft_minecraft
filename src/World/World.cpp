#include "World/World.hpp"
#include "Core/DataBuffer.hpp"
#include "Profiler.hpp"
#include "World/Generator.hpp"
#include "World/Pass/Surface.hpp"
#include "World/Registry.hpp"

World::World(uint64_t seed)
    : m_seed(seed)
{
    Ref<Shader> shader = Shader::load("assets/shaders/voxel.wgsl").value_or(nullptr);
    shader->set_binding("images", Binding(BindingKind::Texture, ShaderStageFlagBits::Fragment, 0, 0, BindingAccess::Read, TextureDimension::D2DArray)); // binding = 1 is the sampler

    shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});
    shader->add_push_constant_range(PushConstantRange(ShaderStageFlagBits::Vertex, sizeof(glm::mat4) * 2 + sizeof(float)));

    m_material = Material::create(shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back, UVType::UVT);
    m_material->set_param("images", BlockRegistry::get_texture_array());

    // Setup world generation
    m_generators[overworld] = newobj(Generator, this, overworld);
    m_generators[overworld]->set_distance(8);
    m_generators[overworld]->add_pass(newobj(SurfacePass));
}

void World::tick(float delta)
{
    ZoneScoped;

    for (Ref<Entity> entity : m_dims[overworld].get_entities())
        entity->recurse_tick(delta);

    const glm::vec3 player_pos = m_camera->get_global_transform().position();
    m_generators[overworld]->set_reference_pos(player_pos);
    m_generators[overworld]->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));
}

void World::draw(RenderPassEncoder& encoder)
{
    ZoneScoped;

    std::lock_guard<std::mutex> guard(m_dims[0].mutex());

    DataBuffer push_constants(sizeof(glm::mat4) * 2 + sizeof(float));

    encoder.bind_material(m_material);

    for (const Ref<Chunk>& chunk : m_dims[0].get_visible_chunks())
    {
        ZoneScopedN("draw.iterate");

        ChunkPos pos = chunk->pos();
        AABB aabb = AABB(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)pos.y * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0),
                         glm::vec3(Chunk::width / 2.0, Chunk::width / 2, Chunk::width / 2));

        if (!m_camera->frustum().contains(aabb))
            continue;

        const glm::mat4& view_matrix = m_camera->get_view_proj_matrix();
        const glm::mat4 model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos.x * Chunk::width, pos.y * Chunk::width, pos.z * Chunk::width));

        push_constants.clear_keep_capacity();
        push_constants.add(view_matrix);
        push_constants.add(model_matrix);
        push_constants.add(0.0f);

        const Ref<Mesh>& mesh = chunk->get_mesh();

        encoder.bind_index_buffer(mesh->get_buffer(MeshBufferKind::Index));
        encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::Position), 0);
        encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::Normal), 1);
        encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::UV), 2);

        encoder.push_constants(push_constants);
        encoder.draw(mesh->vertex_count(), 1);
    }
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_y, chunk_z);

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
    int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_y, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const size_t chunk_local_y = y >= 0 ? (y % 16) : 16 + (y % 16);
    const size_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    chunk->set_block(chunk_local_x, chunk_local_y, chunk_local_z, state);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t y, int64_t z) const
{
    return m_dims[0].get_chunk(x, y, z);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t y, int64_t z)
{
    return m_dims[0].get_chunk(x, y, z);
}
