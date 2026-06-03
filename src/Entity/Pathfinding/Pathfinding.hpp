#pragma once

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

    void find_path(const glm::vec3& start_pos, const glm::vec3& target_pos);
    Vector<glm::vec3> simplify_path(const Vector<PathNode *>& path);
    bool is_walkable(const glm::ivec3& to, int jump_height);

    Vector<PathNode *> m_path;
    std::unordered_map<glm::ivec3, PathNode *, Ivec3Hash> m_nodes;
    Vector<PathNode *> m_open_set;
    std::unordered_set<glm::ivec3, Ivec3Hash> m_close_set;

private:
    World *m_world = nullptr;

    void retrace_path(PathNode *start_node, PathNode *end_node);
    int get_distance(const PathNode& node_a, const PathNode& node_b);
    Vector<PathNode *> get_neighbors(const PathNode& node);
    PathNode *node_from_world_point(const glm::vec3& world_position);
};