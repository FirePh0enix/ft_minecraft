#pragma once

#include "Core/Math.hpp"
#include "Id.hpp"
#include "Resource/BlockState.hpp"
#include "Resource/Model.hpp"

#include <array>
#include <span>
#include <string>

class Block;
class Mesh;
struct RenderPass;
struct BlockDisplayData;

struct BlockState
{
    uint32_t id;

    constexpr BlockState()
        : id()
    {
    }

    explicit BlockState(uint32_t id)
        : id(id)
    {
    }

    constexpr bool is_air() const
    {
        return id == 0;
    }

    bool operator==(BlockState other) const
    {
        return id == other.id;
    }
};

static_assert(sizeof(BlockState) == sizeof(uint32_t));

enum class Axis
{
    X,
    Y,
    Z
};

enum class FaceKind
{
    North = 0,
    South = 1,
    Up = 2,
    Down = 3,
    West = 4,
    East = 5,
};

struct NeighborFlags
{
    static constexpr uint8_t north = 1 << 0;
    static constexpr uint8_t south = 1 << 1;
    static constexpr uint8_t up = 1 << 2;
    static constexpr uint8_t down = 1 << 3;
    static constexpr uint8_t west = 1 << 4;
    static constexpr uint8_t east = 1 << 5;

    uint8_t value;

    static constexpr uint8_t get_opposite_face(FaceKind face)
    {
        switch (face)
        {
        case FaceKind::North:
            return south;
        case FaceKind::South:
            return north;
        case FaceKind::Up:
            return down;
        case FaceKind::Down:
            return up;
        case FaceKind::West:
            return east;
        case FaceKind::East:
            return west;
        }
    }

    constexpr bool has_opposite(FaceKind face) const
    {
        return value & get_opposite_face(face);
    }
};

class Block
{
public:
    Block(std::string_view path);

    bool has_cullface(FaceKind face);

    /// Add the block data to the chunk mesh.
    void add(glm::i64vec3 position, NeighborFlags neighbors, std::vector<uint32_t>& indices, std::vector<glm::vec3>& vertices, std::vector<glm::vec4>& uvs, std::vector<glm::vec3>& normals);

private:
    std::string m_path;
    BlockStateResource m_blockstate;
    Model m_model;

    // Cached values for faster access than reading through the model files.
    bool m_cullfaces[6]{false};
};
