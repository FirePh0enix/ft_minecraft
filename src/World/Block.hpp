#pragma once

#include <vector>

struct GenericData
{
    uint8_t visibility : 6;
};

struct BlockState
{
    uint16_t id;

    union
    {
        uint16_t raw;
        GenericData generic;
    };

    BlockState()
        : id(0), raw(0)
    {
    }

    BlockState(uint16_t id)
        : id(id), raw(0)
    {
    }

    BlockState(uint16_t id, GenericData generic)
        : id(id), generic(generic)
    {
    }

    inline bool is_air() const
    {
        return id == 0;
    }
};

class Block
{
};

class BlockRegistry
{
public:
    inline Block *get_block(uint16_t id)
    {
        return m_blocks[id];
    }

private:
    std::vector<Block *> m_blocks;
};
