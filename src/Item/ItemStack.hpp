#pragma once

#include "Core/Ref.hpp"
#include "World/Block.hpp"

#include <cstddef>

class ItemStack
{
public:
    ItemStack()
        : m_block(nullptr), m_count(0)
    {
    }

    ItemStack(Ref<Block> block, size_t count = 1)
        : m_block(block), m_count(count)
    {
    }

    size_t count() const { return m_count; }
    void set_count(size_t count)
    {
        m_count = count;
        if (count == 0)
            m_block = nullptr;
    }

    Ref<Block> get_block() const { return m_block; }

private:
    Ref<Block> m_block;
    size_t m_count;
};
