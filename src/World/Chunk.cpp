#include "World/Chunk.hpp"

#include "Core/Alloc.hpp"
#include "Core/Logger.hpp"
#include "Render/Types.hpp"
#include "World/Block.hpp"
#include "World/Registry.hpp"

#include <cstdint>
#include <optional>

Chunk::Chunk(int64_t x, int64_t z)
    : m_x(x), m_z(z)
{
    m_blocks = alloc_array_uninitialized<BlockState>(block_count_with_overlap);
    m_biomes = alloc_array_uninitialized<Biome>(block_count_with_overlap);
    m_slices = alloc_array<Slice>(slice_count);

    m_chunk_buffer = EXPECT(Buffer::create(sizeof(glm::vec3) * slice_count, WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));
    std::array<glm::vec3, slice_count> chunk_data{};
    for (size_t i = 0; i < slice_count; i++)
        chunk_data[i] = glm::vec3(x * Chunk::width, i * Chunk::width, z * Chunk::width);
    m_chunk_buffer->update(View(chunk_data).as_bytes());

    instances += 1;
}

Chunk::~Chunk()
{
    destroy_array_nodestruct(m_blocks, block_count_with_overlap);
    destroy_array_nodestruct(m_biomes, block_count_with_overlap);
    destroy_array(m_slices, slice_count);

    instances -= 1;
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
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, false, block->get_texture_index(Axis::X, false))));
                if (m_blocks[linearize(x + 1, y, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::X, true, block->get_texture_index(Axis::X, true))));

                if (y == 0 || m_blocks[linearize(x, y - 1, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, false, block->get_texture_index(Axis::Y, false))));
                if (y == height || m_blocks[linearize(x, y + 1, z)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Y, true, block->get_texture_index(Axis::Y, true))));

                if (m_blocks[linearize(x, y, z - 1)].is_air())
                    TRY(faces.append(ChunkBlockFace(x, y - slice_y_offset, z, Axis::Z, false, block->get_texture_index(Axis::Z, false))));
                if (m_blocks[linearize(x, y, z + 1)].is_air())
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

    slice.mesh = EXPECT(Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16, UVType::UVT));

    return Result<void>();
}

Result<void> CompressedChunk::compress(Ref<Chunk> chunk)
{
    m_compressed_blocks.clear();
    m_compressed_nodes.clear();

    m_compressed_biome_nodes.clear();
    m_compressed_biomes.clear();

    const BlockState *chunk_blocks = chunk->get_blocks();
    const Biome *chunk_biomes = chunk->get_biomes();

    for (int64_t i = 0; i < Chunk::slice_count; i++)
    {
        size_t node_index = m_compressed_nodes.size();

        ChunkNode node;
        node.same = 1;

        TRY(m_compressed_nodes.append(node));

        BlockState saved_block;

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
                    node2.same = 1;

                    TRY(nodes.append(node2));
                    node2.ptr = m_compressed_blocks.size() + blocks.size();

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
                        nodes.remove_one();
                    }
                }

        if (node.child_mask != 0)
        {
            m_compressed_slice_mask = 1 << i;

            if (!node.same)
            {
                for (BlockState block : blocks)
                    TRY(m_compressed_blocks.append(block));
                for (ChunkNode node : nodes)
                    TRY(m_compressed_nodes.append(node));
            }
            else
            {
                node.ptr = m_compressed_blocks.size();
                TRY(m_compressed_blocks.append(saved_block));
            }

            m_compressed_nodes.get_unchecked(node_index) = node;
        }
        else
        {
            m_compressed_nodes.remove_one();
        }

        // Compress the biome data. Each block has a biome values so the only case available is
        // if a 4x4x4 node has the same value.

        size_t bnode_index = m_compressed_biome_nodes.size();

        ChunkBiomeNode bnode;
        bnode.same = 1;

        TRY(m_compressed_biome_nodes.append(bnode));

        Vector<Biome> biomes;
        Vector<ChunkBiomeNode> biome_nodes;

        std::optional<Biome> saved_biome;

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
                TRY(m_compressed_biomes.append(biome));
            for (ChunkBiomeNode node : biome_nodes)
                TRY(m_compressed_biome_nodes.append(node));
        }
        else
        {
            bnode.ptr = m_compressed_biomes.size();
            TRY(m_compressed_biomes.append(saved_biome.value()));
        }

        m_compressed_biome_nodes.get_unchecked(bnode_index) = bnode;
    }

    return Result<void>();
}

Result<void> CompressedChunk::uncompress(Ref<Chunk> chunk) const
{
    (void)chunk;
    return Result<void>();
}
