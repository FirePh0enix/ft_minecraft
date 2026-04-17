#include "World/Chunk.hpp"

#include "Core/Alloc.hpp"
#include "Core/Logger.hpp"
#include "Profiler.hpp"
#include "Render/Types.hpp"
#include "World/Block.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <cstdint>

Chunk::Chunk(int64_t x, int64_t z, World *world)
    : m_x(x), m_z(z)
{
    m_blocks = alloc_array_uninitialized<BlockState>(block_count_with_overlap);
    m_biomes = alloc_array_uninitialized<Biome>(block_count_with_overlap);
    m_slices = alloc_array<Slice>(slice_count);

    for (size_t i = 0; i < slice_count; i++)
    {
        const glm::mat4 model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x * Chunk::width, i * Chunk::width, z * Chunk::width));
        Slice& slice = m_slices[i];

        slice.model.model_matrix = model_matrix;
        slice.model_buffer = RenderingDriver::get()->create_buffer(sizeof(Model), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest).value_or(nullptr);
        slice.model_buffer->update(View(slice.model).as_bytes());

        slice.material = RenderingDriver::get()->create_material(world->m_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back, UVType::UVT);
        slice.material->set_param("images", BlockRegistry::get_texture_array());
        slice.material->set_param("env", world->m_env_buffer);
        slice.material->set_param("model", slice.model_buffer);
    }
}

Chunk::~Chunk()
{
    destroy_array_nodestruct(m_blocks, block_count_with_overlap);
    destroy_array_nodestruct(m_biomes, block_count_with_overlap);
    destroy_array(m_slices, slice_count);
}

void Chunk::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    m_blocks[linearize(x, y, z)] = state;

    size_t slice = y / width;

    Result<void> result = build_simple_mesh(slice);
    if (result.has_error())
    {
        error("Rebuilding the mesh ended with error:");
        result.error().print();
    }
}

struct ChunkBlockFace
{
    uint8_t x;
    uint8_t y;
    uint8_t z;
    Axis axis;
    bool positive;
    uint32_t texture_index;

    ChunkBlockFace(uint8_t x,
                   uint8_t y,
                   uint8_t z,
                   Axis axis,
                   bool positive,
                   uint32_t texture_index) : x(x), y(y), z(z), axis(axis), positive(positive), texture_index(texture_index) {}
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

Result<void> Chunk::build_simple_mesh(size_t slice_index)
{
    ZoneScoped;

    Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Chunk is empty, nothing to generate.
    if (slice.empty)
    {
        return Result<void>();
    }

    // Let's detect which faces are not hidden.
    Vector<ChunkBlockFace> faces;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (m_blocks[index].is_air())
                    continue;

                const Ref<Block>& block = BlockRegistry::get_block_by_id(m_blocks[index].id);

                if (m_blocks[linearize(x - 1, y, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::X, false, block->get_texture_index(Axis::X, false))));
                if (m_blocks[linearize(x + 1, y, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::X, true, block->get_texture_index(Axis::X, true))));

                if (y == 0 || m_blocks[linearize(x, y - 1, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::Y, false, block->get_texture_index(Axis::Y, false))));
                if (y == height || m_blocks[linearize(x, y + 1, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::Y, true, block->get_texture_index(Axis::Y, true))));

                if (m_blocks[linearize(x, y, z - 1)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::Z, false, block->get_texture_index(Axis::Z, false))));
                if (m_blocks[linearize(x, y, z + 1)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y, z, Axis::Z, true, block->get_texture_index(Axis::Z, true))));
            }
        }
    }

    // No faces are visible, let's skip mesh generation.
    if (faces.empty())
    {
        slice.mesh = nullptr;
        return Result<void>();
    }

    // Now we build a mesh from the faces.
    Vector<uint16_t> indices;
    Vector<glm::vec3> vertices;
    Vector<glm::vec3> uvs;
    Vector<glm::vec3> normals;

    for (const ChunkBlockFace& face : faces)
    {
        uint16_t i0 = vertices.size() + 0;
        uint16_t i1 = vertices.size() + 1;
        uint16_t i2 = vertices.size() + 2;
        uint16_t i3 = vertices.size() + 3;

        TRY(indices.append(i0));
        TRY(indices.append(i1));
        TRY(indices.append(i2));

        TRY(indices.append(i2));
        TRY(indices.append(i3));
        TRY(indices.append(i0));

        const std::array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        TRY(vertices.append(new_vertices[0]));
        TRY(vertices.append(new_vertices[1]));
        TRY(vertices.append(new_vertices[2]));
        TRY(vertices.append(new_vertices[3]));

        TRY(uvs.append(glm::vec3(0.0, 0.0, (double)face.texture_index)));
        TRY(uvs.append(glm::vec3(1.0, 0.0, (double)face.texture_index)));
        TRY(uvs.append(glm::vec3(1.0, 1.0, (double)face.texture_index)));
        TRY(uvs.append(glm::vec3(0.0, 1.0, (double)face.texture_index)));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        TRY(normals.append(normal));
        TRY(normals.append(normal));
        TRY(normals.append(normal));
        TRY(normals.append(normal));
    }

    slice.mesh = Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), IndexType::Uint16, UVType::UVT);

    return Result<void>();
}
