#pragma once

#include "Core/Math.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Node.hpp"
#include "World/World.hpp"
#include <cstddef>
#include <unordered_set>
#include <vector>

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
    bool is_walkable(const glm::ivec3& to, int jump_height);

    std::vector<Node *> m_path;
    size_t m_path_index = 0;

private:
    World *m_world = nullptr;

    std::unordered_map<glm::ivec3, Node *, Ivec3Hash> m_nodes;
    std::vector<Node *> m_open_set;
    std::unordered_set<glm::ivec3, Ivec3Hash> m_close_set;

    void retrace_path(Node *start_node, Node *end_node);
    int get_distance(const Node& node_a, const Node& node_b);
    std::vector<Node *> get_neighbors(const Node& node);
    Node *node_from_world_point(const glm::vec3& world_position);
};