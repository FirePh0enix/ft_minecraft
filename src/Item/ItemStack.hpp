#pragma once

#include <cstddef>
#include <cstdint>

class ItemStack
{
public:
    ItemStack()
        : m_block_id(0), m_count(0)
    {
    }

    ItemStack(uint16_t block_id, size_t count = 1)
        : m_block_id(block_id), m_count(count)
    {
    }

    size_t count() const { return m_count; }
    void set_count(size_t count)
    {
        m_count = count;
        if (count == 0)
            m_block_id = 0;
    }

    uint16_t get_block() const { return m_block_id; }

private:
    uint16_t m_block_id;
    size_t m_count;
};
