#pragma once

#include "Core/Containers/InplaceVector.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Math.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include "World/World.hpp"
#include <cstddef>
#include <cstdint>
#include <unordered_set>

struct Ivec3Hash
{
    std::size_t operator()(const glm::ivec3& v) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);
        std::size_t h3 = std::hash<int>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class Pathfinding
{
public:
    Pathfinding(World *world) : m_world(world) {};

    Result<void> find_path(const glm::vec3& start_pos, const glm::vec3& target_pos);
    Result<Vector<glm::vec3>> simplify_path(const LocalVector<uint32_t>& path);
    bool is_walkable(const glm::ivec3& to, int jump_height);

    LocalVector<uint32_t> m_path;
    LocalVector<PathNode> m_nodes;

private:
    World *m_world = nullptr;

    std::unordered_map<glm::ivec3, uint32_t, Ivec3Hash> m_node_lookup;
    LocalVector<uint32_t> m_open_set;
    std::unordered_set<glm::ivec3, Ivec3Hash> m_close_set;

    Result<void> retrace_path(uint32_t start_index, uint32_t end_index);
    int get_distance(const PathNode& node_a, const PathNode& node_b);
    Result<InplaceVector<uint32_t, 10>> get_neighbors(uint32_t current_index);
    Result<uint32_t> node_from_world_point(const glm::vec3& world_position);
};