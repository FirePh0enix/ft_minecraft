#pragma once

#include "Render/Driver.hpp"
#include "World/Block.hpp"

class World;

struct ChunkPos
{
    int64_t x;
    int64_t z;
};

struct __attribute__((packed)) ChunkLocalPos
{
    uint8_t x : 4;
    uint8_t y;
    uint8_t z : 4;
};

class Chunk
{
public:
    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * width * height;

    Chunk(int64_t x, int64_t z)
        : m_x(x), m_z(z)
    {
        m_blocks.resize(block_count);
    }

    inline BlockState get_block(size_t x, size_t y, size_t z) const
    {
        if (x > 15 || y > 255 || z > 15)
            return BlockState();
        return m_blocks[z * width * height + y * width + x];
    }

    inline void set_block(size_t x, size_t y, size_t z, BlockState state)
    {
        if (z * width * height + y * width + x > block_count)
            return;
        m_blocks[z * width * height + y * width + x] = state;
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

    void compute_full_visibility(const Ref<World>& world);

    void update_instance_buffer(Ref<Buffer> buffer);

private:
    std::vector<BlockState> m_blocks;
    int64_t m_x;
    int64_t m_z;

    uint32_t m_buffer_id;
    uint32_t m_block_count = 0;

    BlockState& get_block_ref(size_t x, size_t y, size_t z)
    {
        return m_blocks[z * width * height + y * width + x];
    }

    inline bool has_block(size_t x, size_t y, size_t z) const
    {
        return !m_blocks[z * width * height + y * width + x].is_air();
    }
};
