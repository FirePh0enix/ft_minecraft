#pragma once

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

private:
    static BlockRegistry singleton;

    // Block ids that use the `generic` variant.
    std::set<uint16_t> m_generics;
};
