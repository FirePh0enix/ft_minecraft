#pragma once

#include "Core/Class.hpp"

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

class Block : public Object
{
    CLASS(Block, Object);

public:
    inline std::string name() const
    {
        return m_name;
    }

private:
    std::string m_name;
};
