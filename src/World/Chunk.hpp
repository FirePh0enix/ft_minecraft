#pragma once

#include "Core/Containers/Vector.hpp"
#include "Core/Definitions.hpp"
#include "Core/Hasher.hpp"
#include "Render/Renderer.hpp"
#include "World/Biome.hpp"
#include "World/Block.hpp"

#include <cstdint>

class World;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    constexpr ChunkPos() : x(0), z(0) {}
    constexpr ChunkPos(int64_t x, int64_t z) : x(x), z(z) {}

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    bool operator==(const ChunkPos& other) const
    {
        return std::tie(x, z) == std::tie(other.x, other.z);
    }
};

template <>
struct Hasher<ChunkPos>
{
    uint64_t operator()(const ChunkPos& i)
    {
        uint64_t x = *(uint64_t *)&i.x;
        uint64_t z = *(uint64_t *)&i.z;
        return x * 3 + z * 5;
    }
};

struct ChunkModel
{
    glm::mat4 model_matrix = glm::identity<glm::mat4>();
};

struct __attribute__((packed)) ChunkNode
{
    uint32_t leaf : 1 = 0;
    /**
     * If `same` is set this means all children are the same block. For a leaf block `ptr` points *one* block.
     * For a non-leaf block this means no child nodes are present and `ptr` points to the block.
     * For `same` to be valid all bits of `child_mask` must be set.
     */
    uint32_t same : 1 = 0;
    uint32_t ptr : 30 = 0;
    uint64_t child_mask = 0;
};

struct __attribute__((packed)) ChunkBiomeNode
{
    uint16_t ptr = 0;
    /**
     * All children of this node are the same value. If set then the children nodes
     * are ignored and `ptr` points to the biome value.
     */
    uint16_t same : 1 = 0;
    uint16_t reserved : 15 = 0;
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    struct Slice
    {
        Ref<Mesh> mesh = nullptr;
        bool empty = true;

        bool is_visible() const
        {
            // `empty` means the slice is only air blocks.
            // `mesh.is_null()` means the mesh was not created because no faces were visible.
            return !empty && !mesh.is_null();
        }
    };

    static constexpr int64_t width = 16;
    // static constexpr int64_t width_with_overlap = width + 2;
    static constexpr int64_t height = 256;
    // static constexpr int64_t block_count_with_overlap = width_with_overlap * height * width_with_overlap;
    static constexpr int64_t block_count = width * height * width;
    static constexpr int64_t slice_count = height / width;

    Chunk(int64_t x, int64_t z);
    ~Chunk();

    ALWAYS_INLINE BlockState get_block(int64_t x, int64_t y, int64_t z) const { return m_blocks[linearize(x, y, z)]; }
    void set_block(int64_t x, int64_t y, int64_t z, BlockState state);

    ALWAYS_INLINE const BlockState *get_blocks() const { return m_blocks; }
    ALWAYS_INLINE BlockState *get_blocks() { return m_blocks; }

    ALWAYS_INLINE const Biome *get_biomes() const { return m_biomes; }
    ALWAYS_INLINE Biome *get_biomes() { return m_biomes; }

    ALWAYS_INLINE int64_t x() const { return m_x; }
    ALWAYS_INLINE int64_t z() const { return m_z; }

    ALWAYS_INLINE ChunkPos pos() const { return ChunkPos(m_x, m_z); }

    const Slice *get_slices() const { return m_slices; }
    Slice *get_slices() { return m_slices; }

    Ref<Buffer> get_chunk_buffer() const { return m_chunk_buffer; }

    Result<void> build_simple_mesh(size_t slice);

    static ALWAYS_INLINE size_t linearize(int64_t x, int64_t y, int64_t z) { return z * width * height + y * width + x; }
    // static ALWAYS_INLINE size_t linearize_with_overlap(int64_t x, int64_t y, int64_t z) { return z * width * height + y * width + x; }

private:
    BlockState *m_blocks;
    Biome *m_biomes;
    Slice *m_slices;

    Ref<Buffer> m_chunk_buffer;

    int64_t m_x;
    int64_t m_z;

    bool modified : 1 = false;
};

struct CompressedChunk
{
public:
    CompressedChunk()
        : compressed_slice_mask(0)
    {
    }

    CompressedChunk(Vector<BlockState> compressed_blocks, Vector<ChunkNode> compressed_nodes, uint16_t compressed_slice_mask, Vector<Biome> compressed_biomes, Vector<ChunkBiomeNode> compressed_biome_nodes)
        : compressed_blocks(compressed_blocks), compressed_nodes(compressed_nodes), compressed_slice_mask(compressed_slice_mask), compressed_biomes(compressed_biomes), compressed_biome_nodes(compressed_biome_nodes)
    {
    }

    Result<void> compress(Ref<Chunk> chunk);
    Result<void> uncompress(Ref<Chunk> chunk) const;

    //     View<BlockState> get_blocks() const { return m_compressed_blocks; }
    //     View<ChunkNode> get_nodes() const { return m_compressed_nodes; }

    //     View<Biome> get_biomes() const { return m_compressed_biomes; }
    //     View<ChunkBiomeNode> get_biome_nodes() const { return m_compressed_biome_nodes; }

    // private:
    Vector<BlockState> compressed_blocks;
    Vector<ChunkNode> compressed_nodes;
    uint16_t compressed_slice_mask;

    Vector<Biome> compressed_biomes;
    Vector<ChunkBiomeNode> compressed_biome_nodes;
};
