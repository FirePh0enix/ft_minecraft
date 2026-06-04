#pragma once

#include "Core/Containers/Map.hpp"
#include "Core/Math.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include "World/World.hpp"
#include <cstddef>

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

    void find_path(const glm::vec3& start_pos, const glm::vec3& target_pos);
    Vector<glm::vec3> simplify_path(const Vector<size_t>& path);
    bool is_walkable(const glm::ivec3& to, int jump_height);

    Vector<size_t> m_path;
    Vector<size_t> m_open_set;

    Vector<PathNode> m_node_pool;

    Map<glm::ivec3, size_t> m_nodes;

    Set<glm::ivec3> m_close_set;

private:
    World *m_world = nullptr;

    void retrace_path(size_t start_index, size_t end_index);
    int get_distance(const PathNode& node_a, const PathNode& node_b);
    Vector<size_t> get_neighbors(size_t node_index);
    size_t node_from_world_point(const glm::ivec3& world_position);
};