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

    m_uniform_buffer = EXPECT(Buffer::create(sizeof(FwChunkUniforms) * slice_count, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    Array<FwChunkUniforms, slice_count> uniform_data{};

    for (size_t i = 0; i < slice_count; i++)
    {
        m_slices[i].mesh_bg = BindGroup::create(Renderer::get().get_fw_chunk_shader());
        m_slices[i].mesh_bg->set_param("chunk", m_uniform_buffer, i * sizeof(FwChunkUniforms), sizeof(FwChunkUniforms));
        m_slices[i].mesh_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].mesh_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].mesh_bg->set_param("images", Engine::get().registry().get_texture_array());
        m_slices[i].mesh_bg->set_param("shadowmap", Renderer::get().get_fw_shadowmap());

        m_slices[i].water_bg = BindGroup::create(Renderer::get().get_fw_water_shader());
        m_slices[i].water_bg->set_param("chunk", m_uniform_buffer, i * sizeof(FwChunkUniforms), sizeof(FwChunkUniforms));
        m_slices[i].water_bg->set_param("camera", Renderer::get().get_fw_camera());
        m_slices[i].water_bg->set_param("world_env", Renderer::get().get_fw_world_env());
        m_slices[i].water_bg->set_param("image", Renderer::get().get_fw_water_texture());
        m_slices[i].water_bg->set_param("shadowmap", Renderer::get().get_fw_shadowmap());

        m_slices[i].mesh_shadowmap_bg = BindGroup::create(Renderer::get().get_fw_shadowmap_shader());
        m_slices[i].mesh_shadowmap_bg->set_param("chunk", m_uniform_buffer, i * sizeof(FwChunkUniforms), sizeof(FwChunkUniforms));
        m_slices[i].mesh_shadowmap_bg->set_param("camera", Renderer::get().get_fw_shadowmap_camera());

        uniform_data[i].model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x * Chunk::width, i * Chunk::width, z * Chunk::width));
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

    if (!state.is_air())
        m_slices[slice].empty = false;
    m_modified = true;

    EXPECT(build_simple_mesh(slice));

    if (y > 0 && y % width == 0)
    {
        EXPECT(build_simple_mesh(slice - 1));
    }
    else if (y < Chunk::height - 1 && y % width == width - 1)
    {
        EXPECT(build_simple_mesh(slice + 1));
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

Result<void> Chunk::build_simple_mesh(size_t slice_index)
{
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

                Ref<Block> block = Engine::get().registry().get_block(m_blocks[index].id);

                if (x == 0 || m_blocks[linearize(x - 1, y, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, block->get_texture_index(Axis::X, false)));
                if (x == Chunk::width - 1 || m_blocks[linearize(x + 1, y, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, block->get_texture_index(Axis::X, true)));

                if (y == 0 || m_blocks[linearize(x, y - 1, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, block->get_texture_index(Axis::Y, false)));
                if (y == height - 1 || m_blocks[linearize(x, y + 1, z)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, block->get_texture_index(Axis::Y, true)));

                if (z == 0 || m_blocks[linearize(x, y, z - 1)].is_air())
                    faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, block->get_texture_index(Axis::Z, false)));
                if (z == Chunk::width - 1 || m_blocks[linearize(x, y, z + 1)].is_air())
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

Result<void> Chunk::build_water_mesh(size_t slice_index)
{
    Slice& slice = m_slices[slice_index];
    int64_t slice_y_offset = int64_t(slice_index) * width;

    // Chunk is empty, nothing to generate.
    if (slice.empty)
    {
        return Result<void>();
    }

    Map<ChunkPos, Ref<Chunk>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_dim->mutex());
        Array<ChunkPos, 4> array{
            ChunkPos(m_x - 1, m_z),
            ChunkPos(m_x + 1, m_z),
            ChunkPos(m_x, m_z - 1),
            ChunkPos(m_x, m_z + 1),
        };

        for (const auto& pos : array)
        {
            Option<Ref<Chunk>> chunk_opt = m_dim->get_chunk(pos.x, pos.z);
            if (chunk_opt.has_value())
                chunks.put(pos, chunk_opt.value());
        }
    }

    auto check_neighbour_block = [chunks](int64_t cx, int64_t cz, int64_t x, int64_t y, int64_t z)
    {
        Option<Ref<Chunk>> chunk_opt = chunks.get(ChunkPos(cx, cz));
        if (chunk_opt.has_value())
        {
            Ref<Chunk> chunk = chunk_opt.value();
            return chunk->get_block(x, y, z).is_air() && !chunk->get_tag(glm::i64vec3(width - 1, y, z), "water");
        }
        return true;
    };

    // Let's detect which faces are not hidden.
    Vector<ChunkBlockFace> faces;
    {
        for (int64_t x = 0; x < Chunk::width; x++)
        {
            for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
            {
                for (int64_t z = 0; z < Chunk::width; z++)
                {
                    const uint32_t index = linearize(x, y, z);

                    if (!get_tag(index, "water").has_value())
                        continue;

                    if ((x == 0 && check_neighbour_block(m_x - 1, m_z, width - 1, y, z)) || (x > 0 && get_block(x - 1, y, z).is_air() && !get_tag(glm::i64vec3(x - 1, y, z), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, 0));
                    if ((x == width - 1 && check_neighbour_block(m_x + 1, m_z, 0, y, z)) || (x < width - 1 && get_block(x + 1, y, z).is_air() && !get_tag(glm::i64vec3(x + 1, y, z), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, 0));

                    if (y == 0 || (get_block(x, y - 1, z).is_air() && !get_tag(glm::i64vec3(x, y - 1, z), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, 0));
                    if (y == height - 1 || (get_block(x, y + 1, z).is_air() && !get_tag(glm::i64vec3(x, y + 1, z), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, 0));

                    if ((z == 0 && check_neighbour_block(m_x, m_z - 1, x, y, width - 1)) || (z > 0 && get_block(x, y, z - 1).is_air() && !get_tag(glm::i64vec3(x, y, z - 1), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, 0));
                    if ((z == width - 1 && check_neighbour_block(m_x, m_z, x, y, 0)) || (z < width - 1 && get_block(x, y, z + 1).is_air() && !get_tag(glm::i64vec3(x, y, z + 1), "water").has_value()))
                        faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, 0));
                }
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

// Result<CompressedChunk> Chunk::compress() const
// {
//     CompressedChunk cchunk{};

//     const BlockState *chunk_blocks = get_blocks();
//     const Biome *chunk_biomes = get_biomes();

//     for (int64_t i = 0; i < Chunk::slice_count; i++)
//     {
//         size_t node_index = cchunk.compressed_nodes.size();

//         ChunkNode node;
//         node.same = 0;

//         cchunk.compressed_nodes.append(node);

//         BlockState saved_block = m_blocks[(i * 16 + 0) + 0];

//         Vector<BlockState> blocks;
//         Vector<ChunkNode> nodes;

//         for (int64_t xx = 0; xx < 4; xx++)
//             for (int64_t yy = 0; yy < 4; yy++)
//                 for (int64_t zz = 0; zz < 4; zz++)
//                 {
//                     uint64_t index = xx + yy * 4 + zz * 16;

//                     size_t node2_index = nodes.size();
//                     ChunkNode node2;
//                     node2.leaf = 1;
//                     node2.same = 0;

//                     nodes.append(node2);
//                     node2.ptr = cchunk.compressed_blocks.size() + blocks.size();

//                     BlockState saved_block2 = chunk_blocks[Chunk::linearize(0, i * 16, 0)];

//                     Vector<BlockState> blocks2;

//                     for (int64_t xx2 = 0; xx2 < 4; xx2++)
//                         for (int64_t yy2 = 0; yy2 < 4; yy2++)
//                             for (int64_t zz2 = 0; zz2 < 4; zz2++)
//                             {
//                                 uint64_t index2 = xx2 + yy2 * 4 + zz2 * 16;
//                                 BlockState block = chunk_blocks[Chunk::linearize(xx * 4 + xx2, yy * 4 + yy2 + i * 16, zz * 4 + zz2)];

//                                 if (block != saved_block2)
//                                     node2.same = 0;

//                                 if (block.is_air())
//                                     continue;

//                                 blocks2.append(block);
//                                 node2.child_mask |= 1 << index2;
//                             }

//                     if (node2.child_mask != 0)
//                     {
//                         node.child_mask |= 1 << index;
//                         nodes.get_unchecked(node2_index) = node2;

//                         if (!node2.same)
//                         {
//                             for (BlockState block : blocks2)
//                                 blocks.append(block);

//                             node.same = 0;
//                         }
//                         else
//                         {
//                             blocks.append(saved_block2);

//                             if (saved_block.is_air())
//                                 saved_block = saved_block2;

//                             if (saved_block != saved_block2)
//                                 node.same = 0;
//                         }
//                     }
//                     else
//                     {
//                         nodes.remove_one();
//                     }
//                 }

//         if (node.child_mask != 0)
//         {
//             cchunk.compressed_slice_mask = 1 << i;

//             if (!node.same)
//             {
//                 for (BlockState block : blocks)
//                     cchunk.compressed_blocks.append(block);
//                 for (ChunkNode node : nodes)
//                     cchunk.compressed_nodes.append(node);
//             }
//             else
//             {
//                 node.ptr = cchunk.compressed_blocks.size();
//                 cchunk.compressed_blocks.append(saved_block);
//             }

//             cchunk.compressed_nodes.get_unchecked(node_index) = node;
//         }
//         else
//         {
//             cchunk.compressed_nodes.remove_one();
//         }

//         // Compress the biome data. Each block has a biome values so the only case available is
//         // if a 4x4x4 node has the same value.

//         size_t bnode_index = cchunk.compressed_biome_nodes.size();

//         ChunkBiomeNode bnode;
//         bnode.same = 1;

//         cchunk.compressed_biome_nodes.append(bnode);

//         Vector<Biome> biomes;
//         Vector<ChunkBiomeNode> biome_nodes;

//         Option<Biome> saved_biome;

//         for (int64_t xx = 0; xx < 4; xx++)
//             for (int64_t yy = 0; yy < 4; yy++)
//                 for (int64_t zz = 0; zz < 4; zz++)
//                 {
//                     ChunkBiomeNode bnode2;
//                     bnode2.same = 1;

//                     biome_nodes.append(bnode2);

//                     Biome saved_biome2 = chunk_biomes[Chunk::linearize(0, i * 16, 0)];
//                     Vector<Biome> biomes2;

//                     for (int64_t xx2 = 0; xx2 < 4; xx2++)
//                         for (int64_t yy2 = 0; yy2 < 4; yy2++)
//                             for (int64_t zz2 = 0; zz2 < 4; zz2++)
//                             {
//                                 Biome biome = chunk_biomes[Chunk::linearize(xx * 4 + xx2, yy * 4 + yy2 + i * 16, zz * 4 + zz2)];

//                                 if (biome != saved_biome2)
//                                     bnode2.same = 0;

//                                 biomes2.append(biome);
//                             }

//                     if (!bnode2.same)
//                     {
//                         for (Biome biome : biomes2)
//                             biomes.append(biome);

//                         bnode.same = 0;
//                     }
//                     else
//                     {
//                         bnode2.ptr = biomes.size();
//                         biomes.append(saved_biome2);

//                         if (!saved_biome.has_value())
//                             saved_biome = saved_biome2;

//                         if (saved_biome != saved_biome2)
//                             bnode.same = 0;
//                     }
//                 }

//         if (!bnode.same)
//         {
//             for (Biome biome : biomes)
//                 cchunk.compressed_biomes.append(biome);
//             for (ChunkBiomeNode node : biome_nodes)
//                 cchunk.compressed_biome_nodes.append(node);
//         }
//         else
//         {
//             bnode.ptr = cchunk.compressed_biomes.size();
//             cchunk.compressed_biomes.append(saved_biome.value());
//         }

//         cchunk.compressed_biome_nodes.get_unchecked(bnode_index) = bnode;
//     }

//     return cchunk;
// }

// Result<void> Chunk::uncompress(View<BlockState> compressed_blocks, View<ChunkNode> compressed_nodes, uint16_t compressed_slice_mask, View<Biome> compressed_biomes, View<ChunkBiomeNode> compressed_biome_nodes)
// {
//     (void)compressed_biomes;
//     (void)compressed_biome_nodes;

//     size_t cursor = 0;

//     for (int64_t slice_y = 0; slice_y < Chunk::slice_count; slice_y++)
//     {
//         if (((compressed_slice_mask >> slice_y) & 1) == 0)
//         {
//             continue;
//         }

//         ChunkNode node = compressed_nodes[cursor];
//         // if (node.same)
//         // {
//         //     m_slices[slice_y].empty = false;

//         //     BlockState bs = compressed_blocks[node.ptr];
//         //     for (int64_t x = 0; x < Chunk::width; x++)
//         //         for (int64_t y = 0; y < Chunk::width; y++)
//         //             for (int64_t z = 0; z < Chunk::width; z++)
//         //                 m_blocks[Chunk::linearize(x, y + slice_y * 16, z)] = bs;
//         // }
//         // else
//         // {
//         for (int64_t x = 0; x < 4; x++)
//             for (int64_t y = 0; y < 4; y++)
//                 for (int64_t z = 0; z < 4; z++)
//                 {
//                     uint64_t index = x + y * 4 + z * 4 * 4;
//                     if (((node.child_mask >> index) & 1) == 0)
//                         continue;

//                     ChunkNode node2 = compressed_nodes[cursor];
//                     // auto leaf = node.leaf;
//                     // auto ptr = node.ptr;
//                     // println("{} {}", leaf, ptr);

//                     // BlockState bs = compressed_blocks[node.ptr];
//                     // for (int64_t x2 = 0; x2 < Chunk::width; x2++)
//                     //     for (int64_t y2 = 0; y2 < Chunk::width; y2++)
//                     //         for (int64_t z2 = 0; z2 < Chunk::width; z2++)
//                     //             m_blocks[Chunk::linearize(x * 4 + x2, (y * 4 + y2) + slice_y * 16, z * 4 + z2)] = bs;

//                     for (int64_t x2 = 0; x2 < 4; x2++)
//                         for (int64_t y2 = 0; y2 < 4; y2++)
//                             for (int64_t z2 = 0; z2 < 4; z2++)
//                             {
//                                 uint64_t index2 = x + y * 4 + z * 4 * 4;
//                                 if (((node2.child_mask >> index2) & 1) == 0)
//                                     continue;

//                                 BlockState bs = compressed_blocks[node2.ptr];
//                                 m_blocks[Chunk::linearize(x * 4 + x2, (y * 4 + y2) + slice_y * 16, z * 4 + z2)] = bs;
//                             }

//                     cursor += std::popcount(node2.child_mask);
//                     // TODO: learn to use popcount
//                 }

//         // cursor += offset; // std::popcount(node.child_mask);
//         // }

//         EXPECT(build_simple_mesh(slice_y));
//         cursor += std::popcount(node.child_mask);
//     }

//     return Result<void>();
// }

void Chunk::set_tag(glm::i64vec3 pos, const StringView& name, Variant v, bool rebuild)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    BlockTags *tags = m_tags.get_or_put(key, BlockTags());
    tags->tags.put(name, v);
    if (rebuild)
        EXPECT(build_water_mesh(pos.y / Chunk::width));
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
            if (rebuild)
                EXPECT(build_water_mesh(pos.y / Chunk::width));
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
