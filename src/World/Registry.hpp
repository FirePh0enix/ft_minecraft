#pragma once

#include "Core/Ref.hpp"
#include "World/Block.hpp"

#include <set>

class BlockRegistry
{
public:
    static BlockRegistry& get()
    {
        return singleton;
    }

    inline bool is_generic(uint16_t id)
    {
        return m_generics.contains(id);
    }

    inline const std::vector<Ref<Block>>& get_blocks()
    {
        return m_blocks;
    }

    inline size_t get_block_count() const
    {
        return m_blocks.size();
    }

private:
    static BlockRegistry singleton;

    std::vector<Ref<Block>> m_blocks;

    // Block ids that use the `generic` variant.
    std::set<uint16_t> m_generics;
};
