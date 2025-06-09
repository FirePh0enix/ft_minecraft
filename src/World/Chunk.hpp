#pragma once

#include "Render/Driver.hpp"
#include "World/Block.hpp"

class Chunk
{
public:
    static constexpr size_t width = 16;
    static constexpr size_t height = 256;
    static constexpr size_t block_count = width * width * height;

    static Chunk flat(ssize_t x, ssize_t z, BlockState state, uint32_t height);

    Chunk(ssize_t x, ssize_t z)
        : m_x(x), m_z(z)
    {
        m_blocks.resize(block_count);
    }

    inline BlockState get_block(size_t x, size_t y, size_t z) const
    {
        return m_blocks[z * width * height + y * width + x];
    }

    inline void set_block(size_t x, size_t y, size_t z, BlockState state)
    {
        m_blocks[z * width * height + y * width + x] = state;
    }

    inline ssize_t x() const
    {
        return m_x;
    }

    inline ssize_t z() const
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

    void update_instance_buffer(Ref<Buffer> buffer);

private:
    std::vector<BlockState> m_blocks;
    ssize_t m_x;
    ssize_t m_z;

    uint32_t m_buffer_id;
    uint32_t m_block_count = 0;
};
