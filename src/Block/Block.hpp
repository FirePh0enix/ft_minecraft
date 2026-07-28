#pragma once

#include "Id.hpp"

#include <array>
#include <span>
#include <string>

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
        return id == other.id;
    }
};

static_assert(sizeof(BlockState) == sizeof(uint16_t));

enum class Axis : uint8_t
{
    X,
    Y,
    Z
};

class Block
{
public:
    virtual ~Block() {}

    Block() : m_transparent(false) {}
    Block(const std::array<std::string, 6>& textures);
    Block(std::string_view texture);

    std::span<const std::string, 6> get_texture_names() const
    {
        return m_textures;
    }

    std::span<const uint32_t, 6> get_texture_ids() const
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

    void set_texture(std::string_view path);

    bool is_tranparent() const { return m_transparent; }

private:
    std::array<std::string, 6> m_textures{};
    std::array<uint32_t, 6> m_texture_ids{0, 0, 0, 0, 0, 0}; // [+Z, -Z, +X, -X, +Y, -Y]
    bool m_transparent;
};
