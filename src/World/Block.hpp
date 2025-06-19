#pragma once

#include "Core/Class.hpp"

enum class BlockStateVariant : uint8_t
{
    // Uses the `generic` field of the block state.
    Generic,
};

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
    Block(std::string name, const std::array<std::string, 6>& textures)
        : m_name(name), m_textures(textures), m_texture_ids({0, 0, 0, 0, 0, 0})
    {
    }

    inline std::string name() const
    {
        return m_name;
    }

    virtual BlockStateVariant get_variant() const
    {
        return BlockStateVariant::Generic;
    }

    void set_texture_ids(const std::array<uint32_t, 6>& texture_ids)
    {
        m_texture_ids = texture_ids;
    }

    std::array<std::string, 6> get_texture_names() const
    {
        return m_textures;
    }

    std::array<uint32_t, 6> get_texture_ids() const
    {
        return m_texture_ids;
    }

private:
    std::string m_name;
    std::array<std::string, 6> m_textures;
    std::array<uint32_t, 6> m_texture_ids;
};
