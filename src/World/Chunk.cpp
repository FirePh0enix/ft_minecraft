#include "World/Chunk.hpp"

#include "Block/Block.hpp"
#include "Core/Alloc.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"
#include "Render/Types.hpp"
#include "World/Registry.hpp"

#include <cstdint>
#include <mutex>

Chunk::Chunk(Dimension *dim, int64_t x, int64_t z)
    : m_dim(dim), m_x(x), m_z(z)
{
    m_blocks = alloc_array_uninitialized<BlockState>(block_count);
    m_biomes = alloc_array_uninitialized<Biome>(block_count);
    m_slices = alloc_array<Slice>(slice_count);

    m_uniform_buffer = EXPECT(Buffer::create(sizeof(FwChunkUniforms) * slice_count, WGPUBufferUsage_Uniform | WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));
    Array<glm::vec3, slice_count> uniform_data{};

    for (size_t i = 0; i < slice_count; i++)
    {
        m_slices[i].mesh_bg = BindGroup::create(Renderer::get().get_fw_chunk_shader());
        m_slices[i].mesh_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].mesh_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].mesh_bg->set_param("images", Engine::get().registry().get_texture_array());
        m_slices[i].mesh_bg->set_param("shadowmap", Renderer::get().get_fw_shadowmap());

        m_slices[i].water_bg = BindGroup::create(Renderer::get().get_fw_water_shader());
        m_slices[i].water_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].water_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].water_bg->set_param("image", Renderer::get().get_fw_water_texture());
        m_slices[i].water_bg->set_param("shadowmap", Renderer::get().get_fw_shadowmap());

        m_slices[i].mesh_shadowmap_bg = BindGroup::create(Renderer::get().get_fw_shadowmap_shader());
        m_slices[i].mesh_shadowmap_bg->set_param("camera", Renderer::get().get_fw_shadowmap_camera());

	uniform_data[i] = glm::vec3(x * Chunk::width, i * Chunk::width, z * Chunk::width);
    }

    m_uniform_buffer->update(View(uniform_data).as_bytes());
}

Chunk::~Chunk()
{
    destroy_array_nodestruct(m_blocks, block_count);
    destroy_array_nodestruct(m_biomes, block_count);
    destroy_array(m_slices, slice_count);
}

void Chunk::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    if (y < 0 || y > Chunk::height)
        return;
    m_blocks[linearize(x, y, z)] = state;

    size_t slice = y / width;

    m_modified = true;

    m_dim->queue_rebuild(ChunkPos(m_x, m_z));
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

static Array<glm::vec3, 4> vertex_from_axis(Axis axis, bool positive, glm::vec3 offset)
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

Result<void> Chunk::build_simple_mesh(size_t slice_index, const Map<ChunkPos, Ref<Chunk>>& chunks)
{
    Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Let's detect which faces are not hidden.
    LocalVector<ChunkBlockFace> faces;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (m_blocks[index].is_air())
                    continue;

                Ref<Block> block = Engine::get().registry().get_block(m_blocks[index].id);
		if (block.is_null()) {
		    return Result<void>();
		}

                if ((x > 0 && m_blocks[linearize(x - 1, y, z)].is_air()) || (x == 0 && chunks.contains(ChunkPos(m_x - 1, m_z)) && chunks.get(ChunkPos(m_x - 1, m_z)).value()->get_block(15, y, z).is_air()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, block->get_texture_index(Axis::X, false)));
                if ((x < 15 && m_blocks[linearize(x + 1, y, z)].is_air()) || (x == 15 && chunks.contains(ChunkPos(m_x + 1, m_z)) && chunks.get(ChunkPos(m_x + 1, m_z)).value()->get_block(0, y, z).is_air()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, block->get_texture_index(Axis::X, true)));

                if (y == 0 || m_blocks[linearize(x, y - 1, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, block->get_texture_index(Axis::Y, false)));
                if (y == height - 1 || m_blocks[linearize(x, y + 1, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, block->get_texture_index(Axis::Y, true)));

                if ((z > 0 && m_blocks[linearize(x, y, z - 1)].is_air()) || (z == 0 && chunks.contains(ChunkPos(m_x, m_z - 1)) && chunks.get(ChunkPos(m_x, m_z - 1)).value()->get_block(x, y, 15).is_air()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, block->get_texture_index(Axis::Z, false)));
                if ((z < 15 && m_blocks[linearize(x, y, z + 1)].is_air()) || (z == 15 && chunks.contains(ChunkPos(m_x, m_z + 1)) && chunks.get(ChunkPos(m_x, m_z + 1)).value()->get_block(x, y, 0).is_air()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, block->get_texture_index(Axis::Z, true)));
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

        indices.append(i0);
        indices.append(i1);
        indices.append(i2);

        indices.append(i2);
        indices.append(i3);
        indices.append(i0);

        const Array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        vertices.append(new_vertices[0]);
        vertices.append(new_vertices[1]);
        vertices.append(new_vertices[2]);
        vertices.append(new_vertices[3]);

        uvs.append(glm::vec3(0.0, 0.0, (double)face.texture_index));
        uvs.append(glm::vec3(1.0, 0.0, (double)face.texture_index));
        uvs.append(glm::vec3(1.0, 1.0, (double)face.texture_index));
        uvs.append(glm::vec3(0.0, 1.0, (double)face.texture_index));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        normals.append(normal);
        normals.append(normal);
        normals.append(normal);
        normals.append(normal);
    }

    slice.mesh = EXPECT(Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16, UVType::UVT));

    return Result<void>();
}

Result<void> Chunk::build_water_mesh(size_t slice_index, const Map<ChunkPos, Ref<Chunk>>& chunks)
{
    Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Let's detect which faces are not hidden.
    Vector<ChunkBlockFace> faces;
    
    for (int64_t x = 0; x < Chunk::width; x++) {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++) {
            for (int64_t z = 0; z < Chunk::width; z++) {
                const uint32_t index = linearize(x, y, z);

                if (!get_tag(index, "water").has_value())
                    continue;

                if ((x > 0 && !get_tag(glm::i64vec3(x - 1, y, z), "water").has_value()) || (x == 0 && chunks.contains(ChunkPos(m_x - 1, m_z)) && !chunks.get(ChunkPos(m_x - 1, m_z)).value()->get_tag(glm::i64vec3(15, y, z), "water").has_value()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, 0));
                if ((x < 15 && !get_tag(glm::i64vec3(x + 1, y, z), "water").has_value()) || (x == 15 && chunks.contains(ChunkPos(m_x + 1, m_z)) && !chunks.get(ChunkPos(m_x + 1, m_z)).value()->get_tag(glm::i64vec3(0, y, z), "water")))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, 0));

                if (y == 0 || (!get_tag(glm::i64vec3(x, y - 1, z), "water").has_value()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, 0));
                if (y == height - 1 || (!get_tag(glm::i64vec3(x, y + 1, z), "water").has_value()))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, 0));

                if ((z > 0 && !get_tag(glm::i64vec3(x, y, z - 1), "water").has_value()) || (z == 0 && chunks.contains(ChunkPos(m_x, m_z - 1)) && !chunks.get(ChunkPos(m_x, m_z - 1)).value()->get_tag(glm::i64vec3(x, y, 0), "water")))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, 0));
		if ((z < 15 && !get_tag(glm::i64vec3(x, y, z + 1), "water").has_value()) || (z == 15 && chunks.contains(ChunkPos(m_x, m_z + 1)) && !chunks.get(ChunkPos(m_x, m_z + 1)).value()->get_tag(glm::i64vec3(x, y, 0), "water")))
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, 0));
            }
        }
    }

    // No faces are visible, let's skip mesh generation.
    if (faces.empty())
    {
        slice.water_mesh = nullptr;
        return Result<void>();
    }

    // Now we build a mesh from the faces.
    Vector<uint16_t> indices;
    Vector<glm::vec3> vertices;
    Vector<glm::vec2> uvs;
    Vector<glm::vec3> normals;

    for (const ChunkBlockFace& face : faces)
    {
        uint16_t i0 = vertices.size() + 0;
        uint16_t i1 = vertices.size() + 1;
        uint16_t i2 = vertices.size() + 2;
        uint16_t i3 = vertices.size() + 3;

        indices.append(i0);
        indices.append(i1);
        indices.append(i2);

        indices.append(i2);
        indices.append(i3);
        indices.append(i0);

        const Array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        vertices.append(new_vertices[0]);
        vertices.append(new_vertices[1]);
        vertices.append(new_vertices[2]);
        vertices.append(new_vertices[3]);

        uvs.append(glm::vec2(0.0, 0.0));
        uvs.append(glm::vec2(1.0, 0.0));
        uvs.append(glm::vec2(1.0, 1.0));
        uvs.append(glm::vec2(0.0, 1.0));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        normals.append(normal);
        normals.append(normal);
        normals.append(normal);
        normals.append(normal);
    }

    slice.water_mesh = EXPECT(Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16, UVType::UV));

    return Result<void>();
}

void Chunk::set_tag(glm::i64vec3 pos, const StringView& name, Variant v, bool rebuild)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    BlockTags *tags = m_tags.get_or_put(key, BlockTags());
    tags->tags.put(name, v);

    m_dim->queue_rebuild(ChunkPos(m_x, m_z));
}

void Chunk::remove_tag(glm::i64vec3 pos, const StringView& name, bool rebuild)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    Option<BlockTags *> tags = m_tags.get_ptr(key);

    if (tags.has_value())
    {
        tags.value()->tags.erase(name);

        if (tags.value()->tags.size() == 0)
        {
            m_tags.erase(key);
	    m_dim->queue_rebuild(ChunkPos(m_x, m_z)); // TODO: Add a way to rebuild one slice ? (pos.y / 16)
        }
    }
}

Option<Variant> Chunk::get_tag(uint16_t index, const StringView& name) const
{
    Option<const BlockTags *> tags = m_tags.get_ptr(index);
    if (tags.has_value())
        return tags.value()->tags.get(name);
    return None;
}

Option<Variant> Chunk::get_tag(glm::i64vec3 pos, const StringView& name) const
{
    return get_tag(linearize(pos.x, pos.y, pos.z), name);
}

void Chunk::merge_tag(uint16_t index, const BlockTags& tags)
{
    if (tags.tags.size() == 0) {
	return;
    }

    BlockTags *t = m_tags.get_or_put(index, BlockTags());
    
    for (const auto [_, name, value] : tags.tags) {
	t->tags.put(name, value);
    }
}
