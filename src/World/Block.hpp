#pragma once

#include "Core/Class.hpp"
#include "Core/Definitions.hpp"

enum class GradientType : uint8_t
{
    None,
    Grass,
    Water,
};

struct GenericData
{
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

    ALWAYS_INLINE bool is_air() const
    {
        return id == 0;
    }

    bool operator==(BlockState other) const
    {
        return *(uint32_t *)this == *(uint32_t *)&other;
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
    Block(String name, uint16_t id, const std::array<String, 6>& textures, GradientType gradient_type = GradientType::None, bool transparent = false)
        : m_name(name), m_id(id), m_textures(textures), m_texture_ids({0, 0, 0, 0, 0, 0}), m_gradient_type(gradient_type), m_transparent(transparent)
    {
    }

    inline String name() const
    {
        return m_name;
    }

    uint16_t id() const { return m_id; }

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

    bool is_tranparent() const { return m_transparent; }

private:
    String m_name;
    uint16_t m_id;
    std::array<String, 6> m_textures;
    std::array<uint32_t, 6> m_texture_ids; // [+Z, -Z, +X, -X, +Y, -Y]
    GradientType m_gradient_type;
    bool m_transparent;
};
