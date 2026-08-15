#pragma once

#include "Core/Math.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include "World/World.hpp"

#include <cstddef>
#include <unordered_set>

inline bool operator<(const glm::ivec3& a, const glm::ivec3& b)
{
    if (a.x >= b.x)
        return false;
    if (a.y >= b.y)
        return false;
    return a.z < b.z;
}

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

    void find_path(const glm::vec3& start_pos, const glm::vec3& target_pos, size_t dimension);
    std::vector<glm::vec3> simplify_path(const std::vector<size_t>& path);
    bool is_walkable(const glm::ivec3& to, int jump_height, size_t dimension);

    std::vector<size_t> m_path;
    std::vector<size_t> m_open_set;
    std::vector<PathNode> m_node_pool;
    std::unordered_map<glm::ivec3, size_t, Ivec3Hash> m_nodes;
    std::unordered_set<glm::ivec3, Ivec3Hash> m_close_set;

private:
    World *m_world = nullptr;

    void retrace_path(size_t start_index, size_t end_index);
    int get_distance(const PathNode& node_a, const PathNode& node_b);
    std::vector<size_t> get_neighbors(size_t node_index, size_t dimension);
    size_t node_from_world_point(const glm::ivec3& world_position, int dimension);
};
