#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/String.hpp"
#include "Id.hpp"

enum class GradientType : uint8_t
{
    None,
    Grass,
    Water,
};

class Block;

struct BlockState
{
    Id<Block> id;

    constexpr BlockState()
        : id(0)
    {
    }

    explicit BlockState(Id<Block> id)
        : id(id)
    {
    }

    constexpr bool is_air() const
    {
        return !id.valid();
    }

    bool operator==(BlockState other) const
    {
        return *(uint32_t *)this == *(uint32_t *)&other;
    }
};

static_assert(sizeof(BlockState) == sizeof(uint16_t));

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
    Block() : m_transparent(false) {}
    Block(const Array<String, 6>& textures);
    Block(const StringView& texture);

    Array<String, 6> get_texture_names() const
    {
        return m_textures;
    }

    Array<uint32_t, 6> get_texture_ids() const
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

    void set_texture(const StringView& path);

    bool is_tranparent() const { return m_transparent; }

private:
    Array<String, 6> m_textures;
    Array<uint32_t, 6> m_texture_ids; // [+Z, -Z, +X, -X, +Y, -Y]
    bool m_transparent;
};
