#pragma once

#include "Core/Class.hpp"

enum class GradientType : uint8_t
{
    None,
    Grass,
    Water,
};

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

    constexpr BlockState()
        : id(0), raw(0)
    {
    }

    explicit BlockState(uint16_t id)
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

enum class Axis : uint8_t
{
    X,
    Y,
    Z
};

class Block : public Object
{
    CLASS(Block, Object);

public:
    Block(String name, const std::array<String, 6>& textures, GradientType gradient_type = GradientType::None)
        : m_name(name), m_textures(textures), m_texture_ids({0, 0, 0, 0, 0, 0}), m_gradient_type(gradient_type)
    {
    }

    inline String name() const
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

    std::array<String, 6> get_texture_names() const
    {
        return m_textures;
    }

    std::array<uint32_t, 6> get_texture_ids() const
    {
        return m_texture_ids;
    }

    uint32_t get_texture_index(Axis axis, bool positive) const
    {
        uint32_t index = 0;
        if (axis == Axis::X)
            index = 2 + positive;
        else if (axis == Axis::Y)
            index = 4 + positive;
        else if (axis == Axis::Z)
            index = 0 + positive;

        return m_texture_ids[index];
    }

    GradientType get_gradient_type() const
    {
        return m_gradient_type;
    }

private:
    String m_name;
    std::array<String, 6> m_textures;
    std::array<uint32_t, 6> m_texture_ids; // [+Z, -Z, +X, -X, +Y, -Y]
    GradientType m_gradient_type;
};
