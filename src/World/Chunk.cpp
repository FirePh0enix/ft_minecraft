#include "World/Chunk.hpp"

#include "Block/Block.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"

#include <cstdint>

Chunk::Chunk(Dimension *dim, int64_t x, int64_t z)
    : m_dim(dim), m_x(x), m_z(z)
{
    m_blocks = new BlockState[block_count];
    m_biomes = new Biome[16 * 16];
    m_slices = new Slice[slice_count];

    // m_tags = new std::unordered_map<int64_t, std::map<std::string, std::string>>();
    m_uniform_buffer = EXPECT(Buffer::create(sizeof(FwChunkUniforms) * slice_count, WGPUBufferUsage_Uniform | WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));

    for (size_t i = 0; i < slice_count; i++)
    {
        m_slices[i].mesh_bg = BindGroup::create(Renderer::get().get_fw_chunk_shader());
        m_slices[i].mesh_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].mesh_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].mesh_bg->set_param("images", EXPECT(Engine::get().registry().get_texture_array()->get_view(WGPUTextureViewDimension_2DArray)));
        m_slices[i].mesh_bg->set_param("shadowmap", EXPECT(Renderer::get().get_fw_shadowmap()->get_view()));

        m_slices[i].water_bg = BindGroup::create(Renderer::get().get_fw_water_shader());
        m_slices[i].water_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].water_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].water_bg->set_param("image", EXPECT(Renderer::get().get_fw_water_texture()->get_view()));
        m_slices[i].water_bg->set_param("shadowmap", EXPECT(Renderer::get().get_fw_shadowmap()->get_view()));

        m_slices[i].mesh_shadowmap_bg = BindGroup::create(Renderer::get().get_fw_shadowmap_shader());
        m_slices[i].mesh_shadowmap_bg->set_param("camera", Renderer::get().get_fw_shadowmap_camera());
    }
}

Chunk::~Chunk()
{
    delete[] m_blocks;
    delete[] m_biomes;
    delete[] m_slices;
}

void Chunk::update_instance_buffer(glm::dvec3 position, uint32_t slice_index)
{
    glm::vec3 data((double)m_x * Chunk::width - position.x, (double)slice_index * Chunk::width - position.y, (double)m_z * Chunk::width - position.z);
    m_uniform_buffer->update_struct(data, slice_index * sizeof(glm::vec3));
}

void Chunk::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    if (y < 0 || y > Chunk::height)
        return;

    m_blocks[linearize(x, y, z)] = state;
    m_modified = true;

    m_dim->queue_rebuild(ChunkPos(m_x, m_z));

    if (x == 0)
        m_dim->queue_rebuild(ChunkPos(m_x - 1, m_z));
    else if (x == 15)
        m_dim->queue_rebuild(ChunkPos(m_x + 1, m_z));
    else if (z == 0)
        m_dim->queue_rebuild(ChunkPos(m_x, m_z - 1));
    else if (z == 15)
        m_dim->queue_rebuild(ChunkPos(m_x, m_z + 1));

    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
    if (block != nullptr && !block->is_conventional())
    {
        m_non_conventional_blocks.insert(BlockPos(x, y, z));
    }
    else if (m_non_conventional_blocks.contains(BlockPos(x, y, z)))
    {
        m_non_conventional_blocks.erase(BlockPos(x, y, z));
    }

    // if (y == 0)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), 0, 1);
    // else if (y == 255)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), 15, 1);
    // else if (y % 16 == 0)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16 - 1, 2);
    // else if (y % 16 == 15)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16, 2);
    // else
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16, 1);
}

struct ChunkBlockFace
{
    uint8_t x;
    uint8_t y;
    uint8_t z;
    Axis axis;
    bool positive;
    uint32_t texture_index;
    bool gradient;

    ChunkBlockFace(uint8_t x,
                   uint8_t y,
                   uint8_t z,
                   Axis axis,
                   bool positive,
                   uint32_t texture_index,
                   bool gradient)
        : x(x), y(y), z(z), axis(axis), positive(positive), texture_index(texture_index), gradient(gradient) {}
};

static std::array<glm::vec3, 4> vertex_from_axis(Axis axis, bool positive, glm::vec3 offset)
{
    glm::vec3 v[24]{
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z), // front 0
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),

        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // back 4
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // left 8
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z), // right 12
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z), // top 16
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // bottom 20
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
    };

    if (axis == Axis::X && positive)
        return {v[12], v[13], v[14], v[15]};
    else if (axis == Axis::X)
        return {v[8], v[9], v[10], v[11]};
    else if (axis == Axis::Y && positive)
        return {v[16], v[17], v[18], v[19]};
    else if (axis == Axis::Y)
        return {v[20], v[21], v[22], v[23]};
    else if (axis == Axis::Z && positive)
        return {v[0], v[1], v[2], v[3]};
    else
        return {v[4], v[5], v[6], v[7]};
}

static glm::vec3 normal_from_axis(Axis axis, bool positive)
{
    if (axis == Axis::X)
        return glm::vec3(positive ? 1.0 : -1.0, 0.0, 0.0);
    else if (axis == Axis::Y)
        return glm::vec3(0.0, positive ? 1.0 : -1.0, 0.0);
    else if (axis == Axis::Z)
        return glm::vec3(0.0, 0.0, positive ? 1.0 : -1.0);
    return glm::vec3();
}

Result<std::shared_ptr<Mesh>> Chunk::build_simple_mesh(size_t slice_index, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks)
{
    // Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Let's detect which faces are not hidden.
    std::vector<ChunkBlockFace> faces;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (m_blocks[index].is_air())
                    continue;

                std::shared_ptr<Block> block = Engine::get().registry().get_block(m_blocks[index].id);
                if (block == nullptr)
                    return Result<std::shared_ptr<Mesh>>(nullptr);

                if (!block->is_conventional())
                    continue;

                const bool gradient = block->has_gradient();

                auto match_cross_boundary = [](const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks, int64_t cx, int64_t cz, int64_t x, int64_t y, int64_t z) -> bool
                { 
                    auto iter = chunks.find(ChunkPos(cx, cz));
                    if (iter == chunks.end())
                        return false;
                    BlockState state = iter->second->get_block(x, y, z);
                    if (state.is_air())
                        return true;
                    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
                    if (block != nullptr && !block->is_conventional())
                        return true;
                    return false; };
                auto match = [](BlockState *blocks, int64_t x, int64_t y, int64_t z)
                {
                    BlockState state = blocks[linearize(x, y, z)];
                    if (state.is_air())
                        return true;
                    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
                    if (block != nullptr && !block->is_conventional())
                        return true;
                    return false; };

                if ((x > 0 && match(m_blocks, x - 1, y, z)) || (x == 0 && match_cross_boundary(chunks, m_x - 1, m_z, 15, y, z)))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, block->get_texture_index(Axis::X, false), gradient));
                if ((x < 15 && match(m_blocks, x + 1, y, z)) || (x == 15 && match_cross_boundary(chunks, m_x + 1, m_z, 0, y, z)))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, block->get_texture_index(Axis::X, true), gradient));

                if (y == 0 || match(m_blocks, x, y - 1, z))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, block->get_texture_index(Axis::Y, false), gradient));
                if (y == height - 1 || match(m_blocks, x, y + 1, z))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, block->get_texture_index(Axis::Y, true), gradient));

                if ((z > 0 && match(m_blocks, x, y, z - 1)) || (z == 0 && match_cross_boundary(chunks, m_x, m_z - 1, x, y, 15)))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, block->get_texture_index(Axis::Z, false), gradient));
                if ((z < 15 && match(m_blocks, x, y, z + 1)) || (z == 15 && match_cross_boundary(chunks, m_x, m_z + 1, x, y, 0)))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, block->get_texture_index(Axis::Z, true), gradient));
            }
        }
    }

    // No faces are visible, let's skip mesh generation.
    if (faces.empty())
        return Result<std::shared_ptr<Mesh>>(nullptr);

    // Now we build a mesh from the faces.
    std::vector<uint16_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> uvs;
    std::vector<glm::vec3> normals;

    for (const ChunkBlockFace& face : faces)
    {
        uint16_t i0 = vertices.size() + 0;
        uint16_t i1 = vertices.size() + 1;
        uint16_t i2 = vertices.size() + 2;
        uint16_t i3 = vertices.size() + 3;

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i2);
        indices.push_back(i3);
        indices.push_back(i0);

        const std::array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        vertices.push_back(new_vertices[0]);
        vertices.push_back(new_vertices[1]);
        vertices.push_back(new_vertices[2]);
        vertices.push_back(new_vertices[3]);

        uvs.push_back(glm::vec4(0.0, 0.0, (double)face.texture_index, (float)face.gradient));
        uvs.push_back(glm::vec4(1.0, 0.0, (double)face.texture_index, (float)face.gradient));
        uvs.push_back(glm::vec4(1.0, 1.0, (double)face.texture_index, (float)face.gradient));
        uvs.push_back(glm::vec4(0.0, 1.0, (double)face.texture_index, (float)face.gradient));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
    }

    std::shared_ptr<Mesh> mesh = TRY(Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, normals, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint16, WGPUVertexFormat_Float32x4));
    return mesh;
}

Result<std::shared_ptr<Mesh>> Chunk::build_water_mesh(size_t slice_index, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks)
{
    // Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Let's detect which faces are not hidden.
    std::vector<ChunkBlockFace> faces;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (!get_tag(index, "water").has_value())
                    continue;

                // TODO: add water gradient

                if ((x > 0 && !get_tag(glm::i64vec3(x - 1, y, z), "water").has_value()) || (x == 0 && chunks.contains(ChunkPos(m_x - 1, m_z)) && !chunks.at(ChunkPos(m_x - 1, m_z))->get_tag(glm::i64vec3(15, y, z), "water").has_value()))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, 0, false));
                if ((x < 15 && !get_tag(glm::i64vec3(x + 1, y, z), "water").has_value()) || (x == 15 && chunks.contains(ChunkPos(m_x + 1, m_z)) && !chunks.at(ChunkPos(m_x + 1, m_z))->get_tag(glm::i64vec3(0, y, z), "water")))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, 0, false));

                if (y == 0 || (!get_tag(glm::i64vec3(x, y - 1, z), "water").has_value()))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, 0, false));
                if (y == height - 1 || (!get_tag(glm::i64vec3(x, y + 1, z), "water").has_value()))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, 0, false));

                if ((z > 0 && !get_tag(glm::i64vec3(x, y, z - 1), "water").has_value()) || (z == 0 && chunks.contains(ChunkPos(m_x, m_z - 1)) && !chunks.at(ChunkPos(m_x, m_z - 1))->get_tag(glm::i64vec3(x, y, 0), "water")))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, 0, false));
                if ((z < 15 && !get_tag(glm::i64vec3(x, y, z + 1), "water").has_value()) || (z == 15 && chunks.contains(ChunkPos(m_x, m_z + 1)) && !chunks.at(ChunkPos(m_x, m_z + 1))->get_tag(glm::i64vec3(x, y, 0), "water")))
                    faces.push_back(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, 0, false));
            }
        }
    }

    // No faces are visible, let's skip mesh generation.
    if (faces.empty())
        return Result<std::shared_ptr<Mesh>>(nullptr);

    // Now we build a mesh from the faces.
    std::vector<uint16_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    for (const ChunkBlockFace& face : faces)
    {
        uint16_t i0 = vertices.size() + 0;
        uint16_t i1 = vertices.size() + 1;
        uint16_t i2 = vertices.size() + 2;
        uint16_t i3 = vertices.size() + 3;

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i2);
        indices.push_back(i3);
        indices.push_back(i0);

        const std::array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        vertices.push_back(new_vertices[0]);
        vertices.push_back(new_vertices[1]);
        vertices.push_back(new_vertices[2]);
        vertices.push_back(new_vertices[3]);

        uvs.push_back(glm::vec2(0.0, 0.0));
        uvs.push_back(glm::vec2(1.0, 0.0));
        uvs.push_back(glm::vec2(1.0, 1.0));
        uvs.push_back(glm::vec2(0.0, 1.0));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
    }

    std::shared_ptr<Mesh> water_mesh = TRY(Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, normals, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint16, WGPUVertexFormat_Float32x2));
    return water_mesh;
}

void Chunk::set_tag(glm::i64vec3 pos, std::string_view name, Variant v, bool dont_modify)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    m_tags[key][std::string(name)] = v;
    if (!dont_modify)
        m_modified = true;
}

void Chunk::remove_tag(glm::i64vec3 pos, std::string_view name, bool dont_modify)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    auto tags = m_tags.find(key);

    if (tags != m_tags.end())
    {
        stdext::string_map<Variant>& block_tags = tags->second;
        block_tags.erase(block_tags.find(name));

        if (tags->second.size() == 0)
        {
            m_tags.erase(key);
            if (!dont_modify)
                m_modified = true;
        }
    }
}

std::optional<Variant> Chunk::get_tag(uint16_t index, std::string_view name) const
{
    auto tags = m_tags.find(index);
    if (tags != m_tags.end())
        return tags->second.find(name)->second;
    return std::nullopt;
}

std::optional<Variant> Chunk::get_tag(glm::i64vec3 pos, std::string_view name) const
{
    return get_tag(linearize(pos.x, pos.y, pos.z), name);
}

void Chunk::merge_tag(uint16_t index, const stdext::string_map<Variant>& tags, bool dont_modify)
{
    if (tags.size() == 0)
        return;

    for (const auto& [name, value] : tags)
        m_tags[index][name] = value;

    if (!dont_modify)
        m_modified = true;
}
