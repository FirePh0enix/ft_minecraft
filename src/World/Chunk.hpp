#pragma once

#include "Render/Driver.hpp"
#include "World/Block.hpp"

class World;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }
};

#define CHUNK_LOCAL_X(POS) (POS & 0xF)
#define CHUNK_LOCAL_Y(POS) ((POS >> 4) & 0xFF)
#define CHUNK_LOCAL_Z(POS) ((POS >> 12) & 0xF)

#define CHUNK_POS(X, Y, Z) ((X & 0xF) | ((Y & 0xFF) << 4) | ((Z & 0xFF) << 12))

struct ChunkBounds
{
    glm::ivec3 min;
    glm::ivec3 max;
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * width * height;

    Chunk(int64_t x, int64_t z)
        : m_x(x), m_z(z), m_buffer_id(0)
    {
        m_blocks.resize(block_count);
    }

    inline BlockState get_block(size_t x, size_t y, size_t z) const
    {
        if (x > 15 || y > 255 || z > 15 || (z * width * height + y * width + x) >= m_blocks.size())
            return BlockState();
        return m_blocks[z * width * height + y * width + x];
    }

    inline void set_block(size_t x, size_t y, size_t z, BlockState state)
    {
        if (z * width * height + y * width + x > block_count)
            return;
        m_blocks[z * width * height + y * width + x] = state;
    }

    inline const std::vector<BlockState>& get_blocks() const
    {
        return m_blocks;
    }

    inline int64_t x() const
    {
        return m_x;
    }

    inline int64_t z() const
    {
        return m_z;
    }

    inline uint32_t get_buffer_id() const
    {
        return m_buffer_id;
    }

    inline void set_buffer_id(uint32_t buffer_id)
    {
        m_buffer_id = buffer_id;
    }

    inline uint32_t get_block_count() const
    {
        return m_block_count;
    }

    inline ChunkBounds get_chunk_bounds() const
    {
        return ChunkBounds{.min = glm::ivec3(m_min_x, m_min_y, m_min_z), .max = glm::ivec3(m_max_x, m_max_y, m_max_z)};
    }

    void compute_full_visibility(const Ref<World>& world);
    void compute_visibility(const World *world, int64_t x, int64_t y, int64_t z);

    void compute_axis_neighbour_visibility(const Ref<World>& world, const Ref<Chunk>& neighbour);

    void update_instance_buffer(Ref<Buffer> buffer);

private:
    std::vector<BlockState> m_blocks;
    int64_t m_x;
    int64_t m_z;

    uint32_t m_buffer_id;

    // TODO: transparent blocks
    uint32_t m_block_count = 0;

    uint8_t m_min_x = 15;
    uint8_t m_min_y = 255;
    uint8_t m_min_z = 15;
    uint8_t m_max_x = 0;
    uint8_t m_max_y = 0;
    uint8_t m_max_z = 0;

    BlockState& get_block_ref(size_t x, size_t y, size_t z)
    {
        return m_blocks[z * width * height + y * width + x];
    }

    inline bool has_block(size_t x, size_t y, size_t z) const
    {
        return !m_blocks[z * width * height + y * width + x].is_air();
    }
};
