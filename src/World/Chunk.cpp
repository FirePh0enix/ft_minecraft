#include "World/Chunk.hpp"

#include "Core/Alloc.hpp"
#include "Engine.hpp"
#include "Render/Types.hpp"
#include "World/Block.hpp"
#include "World/Registry.hpp"

#include <bit>
#include <cstdint>

Chunk::Chunk(int64_t x, int64_t z)
    : m_x(x), m_z(z)
{
    m_blocks = alloc_array_uninitialized<BlockState>(block_count);
    m_biomes = alloc_array_uninitialized<Biome>(block_count);
    m_slices = alloc_array<Slice>(slice_count);

    m_chunk_buffer = EXPECT(Buffer::create(sizeof(glm::vec3) * slice_count, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));
    Array<glm::vec3, slice_count> chunk_data{};
    for (size_t i = 0; i < slice_count; i++)
        chunk_data[i] = glm::vec3(x * Chunk::width, i * Chunk::width, z * Chunk::width);
    m_chunk_buffer->update(View(chunk_data).as_bytes());
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
                // if (block->get_texture_ids()[0] == 2)
                //     println("Dwadwadawd");

#define FACE_NEEDS_RENDER(state, block)                                                                       \
    (state.is_air() ||                                                                                        \
     (block->is_tranparent() && !Engine::get().registry().get_block(Id<Block>(state.id))->is_tranparent()) || \
     (!block->is_tranparent() && Engine::get().registry().get_block(Id<Block>(state.id))->is_tranparent()))

                if (x == 0 || FACE_NEEDS_RENDER(m_blocks[linearize(x - 1, y, z)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, block->get_texture_index(Axis::X, false))));
                if (x == Chunk::width - 1 || FACE_NEEDS_RENDER(m_blocks[linearize(x + 1, y, z)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, block->get_texture_index(Axis::X, true))));

                if (y == 0 || FACE_NEEDS_RENDER(m_blocks[linearize(x, y - 1, z)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, block->get_texture_index(Axis::Y, false))));
                if (y == height - 1 || FACE_NEEDS_RENDER(m_blocks[linearize(x, y + 1, z)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, block->get_texture_index(Axis::Y, true))));

                if (z == 0 || FACE_NEEDS_RENDER(m_blocks[linearize(x, y, z - 1)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, block->get_texture_index(Axis::Z, false))));
                if (z == Chunk::width - 1 || FACE_NEEDS_RENDER(m_blocks[linearize(x, y, z + 1)], block))
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, true, block->get_texture_index(Axis::Z, true))));
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

        const Array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
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

    slice.mesh = EXPECT(Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16, UVType::UVT));

    return Result<void>();
}

Result<CompressedChunk> Chunk::compress() const
{
    CompressedChunk cchunk{};

    const BlockState *chunk_blocks = get_blocks();
    const Biome *chunk_biomes = get_biomes();

    for (int64_t i = 0; i < Chunk::slice_count; i++)
    {
        size_t node_index = cchunk.compressed_nodes.size();

        ChunkNode node;
        node.same = 0;

        TRY(cchunk.compressed_nodes.append(node));

        BlockState saved_block = m_blocks[(i * 16 + 0) + 0];

        Vector<BlockState> blocks;
        Vector<ChunkNode> nodes;

        for (int64_t xx = 0; xx < 4; xx++)
            for (int64_t yy = 0; yy < 4; yy++)
                for (int64_t zz = 0; zz < 4; zz++)
                {
                    uint64_t index = xx + yy * 4 + zz * 16;

                    size_t node2_index = nodes.size();
                    ChunkNode node2;
                    node2.leaf = 1;
                    node2.same = 0;

                    TRY(nodes.append(node2));
                    node2.ptr = cchunk.compressed_blocks.size() + blocks.size();

                    BlockState saved_block2 = chunk_blocks[Chunk::linearize(0, i * 16, 0)];

                    Vector<BlockState> blocks2;

                    for (int64_t xx2 = 0; xx2 < 4; xx2++)
                        for (int64_t yy2 = 0; yy2 < 4; yy2++)
                            for (int64_t zz2 = 0; zz2 < 4; zz2++)
                            {
                                uint64_t index2 = xx2 + yy2 * 4 + zz2 * 16;
                                BlockState block = chunk_blocks[Chunk::linearize(xx * 4 + xx2, yy * 4 + yy2 + i * 16, zz * 4 + zz2)];

                                if (block != saved_block2)
                                    node2.same = 0;

                                if (block.is_air())
                                    continue;

                                TRY(blocks2.append(block));
                                node2.child_mask |= 1 << index2;
                            }

                    if (node2.child_mask != 0)
                    {
                        node.child_mask |= 1 << index;
                        nodes.get_unchecked(node2_index) = node2;

                        if (!node2.same)
                        {
                            for (BlockState block : blocks2)
                                TRY(blocks.append(block));

                            node.same = 0;
                        }
                        else
                        {
                            TRY(blocks.append(saved_block2));

                            if (saved_block.is_air())
                                saved_block = saved_block2;

                            if (saved_block != saved_block2)
                                node.same = 0;
                        }
                    }
                    else
                    {
                        TRY(nodes.remove_one());
                    }
                }

        if (node.child_mask != 0)
        {
            cchunk.compressed_slice_mask = 1 << i;

            if (!node.same)
            {
                for (BlockState block : blocks)
                    TRY(cchunk.compressed_blocks.append(block));
                for (ChunkNode node : nodes)
                    TRY(cchunk.compressed_nodes.append(node));
            }
            else
            {
                node.ptr = cchunk.compressed_blocks.size();
                TRY(cchunk.compressed_blocks.append(saved_block));
            }

            cchunk.compressed_nodes.get_unchecked(node_index) = node;
        }
        else
        {
            TRY(cchunk.compressed_nodes.remove_one());
        }

        // Compress the biome data. Each block has a biome values so the only case available is
        // if a 4x4x4 node has the same value.

        size_t bnode_index = cchunk.compressed_biome_nodes.size();

        ChunkBiomeNode bnode;
        bnode.same = 1;

        TRY(cchunk.compressed_biome_nodes.append(bnode));

        Vector<Biome> biomes;
        Vector<ChunkBiomeNode> biome_nodes;

        Option<Biome> saved_biome;

        for (int64_t xx = 0; xx < 4; xx++)
            for (int64_t yy = 0; yy < 4; yy++)
                for (int64_t zz = 0; zz < 4; zz++)
                {
                    ChunkBiomeNode bnode2;
                    bnode2.same = 1;

                    TRY(biome_nodes.append(bnode2));

                    Biome saved_biome2 = chunk_biomes[Chunk::linearize(0, i * 16, 0)];
                    Vector<Biome> biomes2;

                    for (int64_t xx2 = 0; xx2 < 4; xx2++)
                        for (int64_t yy2 = 0; yy2 < 4; yy2++)
                            for (int64_t zz2 = 0; zz2 < 4; zz2++)
                            {
                                Biome biome = chunk_biomes[Chunk::linearize(xx * 4 + xx2, yy * 4 + yy2 + i * 16, zz * 4 + zz2)];

                                if (biome != saved_biome2)
                                    bnode2.same = 0;

                                TRY(biomes2.append(biome));
                            }

                    if (!bnode2.same)
                    {
                        for (Biome biome : biomes2)
                            TRY(biomes.append(biome));

                        bnode.same = 0;
                    }
                    else
                    {
                        bnode2.ptr = biomes.size();
                        TRY(biomes.append(saved_biome2));

                        if (!saved_biome.has_value())
                            saved_biome = saved_biome2;

                        if (saved_biome != saved_biome2)
                            bnode.same = 0;
                    }
                }

        if (!bnode.same)
        {
            for (Biome biome : biomes)
                TRY(cchunk.compressed_biomes.append(biome));
            for (ChunkBiomeNode node : biome_nodes)
                TRY(cchunk.compressed_biome_nodes.append(node));
        }
        else
        {
            bnode.ptr = cchunk.compressed_biomes.size();
            TRY(cchunk.compressed_biomes.append(saved_biome.get()));
        }

        cchunk.compressed_biome_nodes.get_unchecked(bnode_index) = bnode;
    }

    return cchunk;
}

Result<void> Chunk::uncompress(View<BlockState> compressed_blocks, View<ChunkNode> compressed_nodes, uint16_t compressed_slice_mask, View<Biome> compressed_biomes, View<ChunkBiomeNode> compressed_biome_nodes)
{
    (void)compressed_biomes;
    (void)compressed_biome_nodes;

    size_t cursor = 0;

    for (int64_t slice_y = 0; slice_y < Chunk::slice_count; slice_y++)
    {
        if (((compressed_slice_mask >> slice_y) & 1) == 0)
        {
            continue;
        }

        ChunkNode node = compressed_nodes[cursor];
        // if (node.same)
        // {
        //     m_slices[slice_y].empty = false;

        //     BlockState bs = compressed_blocks[node.ptr];
        //     for (int64_t x = 0; x < Chunk::width; x++)
        //         for (int64_t y = 0; y < Chunk::width; y++)
        //             for (int64_t z = 0; z < Chunk::width; z++)
        //                 m_blocks[Chunk::linearize(x, y + slice_y * 16, z)] = bs;
        // }
        // else
        // {
        for (int64_t x = 0; x < 4; x++)
            for (int64_t y = 0; y < 4; y++)
                for (int64_t z = 0; z < 4; z++)
                {
                    uint64_t index = x + y * 4 + z * 4 * 4;
                    if (((node.child_mask >> index) & 1) == 0)
                        continue;

                    ChunkNode node2 = compressed_nodes[cursor];
                    // auto leaf = node.leaf;
                    // auto ptr = node.ptr;
                    // println("{} {}", leaf, ptr);

                    // BlockState bs = compressed_blocks[node.ptr];
                    // for (int64_t x2 = 0; x2 < Chunk::width; x2++)
                    //     for (int64_t y2 = 0; y2 < Chunk::width; y2++)
                    //         for (int64_t z2 = 0; z2 < Chunk::width; z2++)
                    //             m_blocks[Chunk::linearize(x * 4 + x2, (y * 4 + y2) + slice_y * 16, z * 4 + z2)] = bs;

                    for (int64_t x2 = 0; x2 < 4; x2++)
                        for (int64_t y2 = 0; y2 < 4; y2++)
                            for (int64_t z2 = 0; z2 < 4; z2++)
                            {
                                uint64_t index2 = x + y * 4 + z * 4 * 4;
                                if (((node2.child_mask >> index2) & 1) == 0)
                                    continue;

                                BlockState bs = compressed_blocks[node2.ptr];
                                m_blocks[Chunk::linearize(x * 4 + x2, (y * 4 + y2) + slice_y * 16, z * 4 + z2)] = bs;
                            }

                    cursor += std::popcount(node2.child_mask);
                    // TODO: learn to use popcount
                }

        // cursor += offset; // std::popcount(node.child_mask);
        // }

        EXPECT(build_simple_mesh(slice_y));
        cursor += std::popcount(node.child_mask);
    }

    return Result<void>();
}
