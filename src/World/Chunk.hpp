#pragma once

#include "Render/Driver.hpp"
#include "World/Block.hpp"

class World;
class GridCollider;

struct ChunkPos
{
    int64_t x;
    int64_t y;
    int64_t z;

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    bool operator==(const ChunkPos& other) const
    {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    static constexpr int64_t width = 16;
    static constexpr int64_t block_count = width * width * width;

    Chunk(int64_t x, int64_t y, int64_t z);

    ALWAYS_INLINE bool is_empty() const { return m_empty; }

    ALWAYS_INLINE BlockState get_block(int64_t x, int64_t y, int64_t z) const { return m_blocks[linearize(x, y, z)]; }
    ALWAYS_INLINE void set_block(int64_t x, int64_t y, int64_t z, BlockState state) { m_blocks[linearize(x, y, z)] = state; }

    ALWAYS_INLINE const BlockState *get_blocks() const { return m_blocks; }
    ALWAYS_INLINE BlockState *get_blocks() { return m_blocks; }

    ALWAYS_INLINE int64_t x() const { return m_x; }
    ALWAYS_INLINE int64_t y() const { return m_y; }
    ALWAYS_INLINE int64_t z() const { return m_z; }

    ALWAYS_INLINE ChunkPos pos() const { return ChunkPos(m_x, m_y, m_z); }

    void build_simple_mesh();

    ALWAYS_INLINE Ref<Mesh> get_mesh() const { return m_mesh; }

    static ALWAYS_INLINE size_t linearize(int64_t x, int64_t y, int64_t z) { return (z + 1) * (width + 2) * (width + 2) + (y + 1) * (width + 2) + (x + 1); }
    static ALWAYS_INLINE size_t linearize_with_overlap(int64_t x, int64_t y, int64_t z) { return z * (width + 2) * (width + 2) + y * (width + 2) + x; }

private:
    BlockState m_blocks[(Chunk::width + 2) * (Chunk::width + 2) * (Chunk::width + 2)];

    int64_t m_x;
    int64_t m_y;
    int64_t m_z;

    bool m_empty = true;

    // New Mesher
    Ref<Mesh> m_mesh;
};
