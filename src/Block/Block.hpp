#pragma once

#include "Core/Math.hpp"
#include "Id.hpp"

#include <array>
#include <memory>
#include <span>
#include <string>

class Block;
class Mesh;
struct RenderPass;

struct BlockState
{
    RuntimeId<Block> id;

    constexpr BlockState()
        : id()
    {
    }

    explicit BlockState(RuntimeId<Block> id)
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

    Block() : m_transparent(false), m_gradient(false) {}
    Block(const std::array<std::string, 6>& textures, bool gradient = false);
    Block(std::string_view texture, bool gradient = false);

    void set_runtime_id(RuntimeId<Block> id) { m_id = id; }

    virtual void draw(const RenderPass& pass, glm::i64vec3 position)
    {
        (void)pass;
        (void)position;
    };

    virtual BlockState get_default_state() const { return BlockState(m_id); }
    bool is_conventional() const { return m_mesh == nullptr; }
    bool is_unbreakable() const { return m_unbreakable; }

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

    bool is_transparent() const { return m_transparent; }
    bool has_gradient() const { return m_gradient; }
    bool is_solid() const { return m_solid; }

protected:
    std::array<std::string, 6> m_textures{};
    std::array<uint32_t, 6> m_texture_ids{0, 0, 0, 0, 0, 0}; // [+Z, -Z, +X, -X, +Y, -Y]
    bool m_transparent;
    bool m_gradient;
    RuntimeId<Block> m_id;

    std::shared_ptr<Mesh> m_mesh;
    bool m_unbreakable = false;
    bool m_solid = true;
};
