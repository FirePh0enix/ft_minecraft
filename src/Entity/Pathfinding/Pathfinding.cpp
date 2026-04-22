#include "Pathfinding.hpp"
#include "Core/Print.hpp"
#include <cstddef>

constexpr int straight = 10;
constexpr int diag_xz = 14;
constexpr int vertical = 18;

Node *Pathfinding::node_from_world_point(const glm::vec3& pos)
{
    glm::ivec3 key(std::floor(pos.x), std::floor(pos.y), std::floor(pos.z));

    auto it = m_nodes.find(key);
    if (it != m_nodes.end())
        return it->second;

    bool walkable = m_world->get_block_state(key.x, key.y, key.z).is_air();
    Node *node = new Node(walkable, glm::vec3(key), key);
    node->m_g_cost = std::numeric_limits<int>::max();
    node->m_h_cost = 0;
    node->m_parent = nullptr;

    m_nodes[key] = node;
    return node;
}

bool Pathfinding::is_walkable(const glm::ivec3& to, int max_jump_height)
{

    bool block_at_to = !m_world->get_block_state(to.x, to.y, to.z).is_air();
    bool block_below_to = !m_world->get_block_state(to.x, to.y - 1, to.z).is_air();

    // Ensure he can stand on.
    if (!block_at_to && block_below_to)
        return true;

    // Ensure he can move to 4 directions while in air / mid jump.
    if (!block_at_to && !block_below_to && max_jump_height > 0)
        return true;

    return false;
}

std::vector<Node *> Pathfinding::get_neighbors(const Node& node)
{
    std::vector<Node *> neighbors;

    static const std::vector<glm::ivec3> directions = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 0, 1},
        {0, 0, -1},

        {-1, 0, 1},
        {1, 0, 1},
        {-1, 0, -1},
        {1, 0, -1},

        {0, 1, 0},
        {0, -1, 0},
    };

    for (const glm::ivec3& dir : directions)
    {
        glm::ivec3 neighbor_pos = node.m_gridPos + dir;

        // Track air time so A* won't create infinite neighbors and explore only to where Mob can jump.
        int air_time = node.m_air_time;
        bool on_ground = !m_world->get_block_state(node.m_gridPos.x, node.m_gridPos.y - 1, node.m_gridPos.z).is_air();
        if (on_ground)
            air_time = 0;

        int remaining_jump = 1 - air_time;

        if (!is_walkable(neighbor_pos, remaining_jump))
            continue;

        glm::ivec3 d = neighbor_pos - node.m_gridPos;

        // diagonal in XZ.
        if (d.y == 0 && std::abs(d.x) == 1 && std::abs(d.z) == 1)
        {
            glm::ivec3 side1(node.m_gridPos.x + d.x, node.m_gridPos.y, node.m_gridPos.z);
            glm::ivec3 side2(node.m_gridPos.x, node.m_gridPos.y, node.m_gridPos.z + d.z);

            // if either side cell is occupied, don't cut the corner.
            if (!m_world->get_block_state(side1.x, side1.y, side1.z).is_air() ||
                !m_world->get_block_state(side2.x, side2.y, side2.z).is_air())
                continue;
        }

        Node *neighbor_node = nullptr;
        auto it = m_nodes.find(neighbor_pos);
        if (it != m_nodes.end())
            neighbor_node = it->second;
        else
        {
            neighbor_node = new Node(true, glm::vec3(neighbor_pos), neighbor_pos);
            bool neighbor_on_ground = !m_world->get_block_state(neighbor_pos.x, neighbor_pos.y - 1, neighbor_pos.z).is_air();
            neighbor_node->m_air_time = neighbor_on_ground ? 0 : air_time + 1;

            m_nodes[neighbor_pos] = neighbor_node;
        }

        neighbors.push_back(neighbor_node);
    }

    return neighbors;
}

int Pathfinding::get_distance(const Node& a, const Node& b)
{
    int dx = std::abs(a.m_gridPos.x - b.m_gridPos.x);
    int dz = std::abs(a.m_gridPos.z - b.m_gridPos.z);
    int dy = std::abs(a.m_gridPos.y - b.m_gridPos.y);

    int diag_dir = std::min(dx, dz);
    int straight_dir = std::max(dx, dz) - diag_dir;

    int horizontal = diag_xz * diag_dir + straight * straight_dir;
    return horizontal + vertical * dy;
}

void Pathfinding::retrace_path(Node *start_node, Node *end_node)
{
    m_path.clear();

    Node *current_node = end_node;

    while (current_node != nullptr && current_node != start_node)
    {
        m_path.push_back(current_node);
        current_node = current_node->m_parent;
    }

    if (current_node == start_node)
        m_path.push_back(start_node);

    std::reverse(m_path.begin(), m_path.end());

    println("-- Path Found --");
    for (size_t i = 0; i < m_path.size(); i++)
    {
        const auto pos = m_path[i]->m_position;
        println("[{} {} {}]", pos.x, pos.y, pos.z);
    }
}

void Pathfinding::find_path(const glm::vec3& start_pos, const glm::vec3& target_pos)
{
    m_open_set.clear();
    m_close_set.clear();
    m_nodes.clear();
    m_path.clear();
    m_path_index = 0;

    Node *start_node = node_from_world_point(start_pos);
    Node *target_node = node_from_world_point(target_pos);

    start_node->m_g_cost = 0;
    start_node->m_h_cost = get_distance(*start_node, *target_node);

    m_open_set.push_back(start_node);

    while (!m_open_set.empty())
    {
        Node *current_node = m_open_set[0];
        for (size_t i = 1; i < m_open_set.size(); ++i)
        {
            if (m_open_set[i]->get_f_cost() < current_node->get_f_cost() ||
                (m_open_set[i]->get_f_cost() == current_node->get_f_cost() &&
                 m_open_set[i]->m_h_cost < current_node->m_h_cost))
                current_node = m_open_set[i];
        }

        m_open_set.erase(std::remove(m_open_set.begin(), m_open_set.end(), current_node), m_open_set.end());
        m_close_set.insert(current_node->m_gridPos);

        if (current_node->m_gridPos == target_node->m_gridPos)
        {
            retrace_path(start_node, current_node);
            return;
        }

        for (Node *neighbor : get_neighbors(*current_node))
        {

            if (!neighbor->m_walkable || m_close_set.contains(neighbor->m_gridPos))
                continue;

            int new_cost = current_node->m_g_cost + get_distance(*current_node, *neighbor);
            if (new_cost < neighbor->m_g_cost ||
                std::find(m_open_set.begin(), m_open_set.end(), neighbor) == m_open_set.end())
            {
                neighbor->m_g_cost = new_cost;
                neighbor->m_h_cost = get_distance(*neighbor, *target_node);
                neighbor->m_parent = current_node;

                if (std::find(m_open_set.begin(), m_open_set.end(), neighbor) == m_open_set.end())
                    m_open_set.push_back(neighbor);
            }
        }
    }

    println("Pathfinding::find_path() cannot reach target. start: [{} {} {}], target: [{} {} {}]", start_node->m_position.x, start_node->m_position.y, start_node->m_position.z, target_node->m_position.x, target_node->m_position.y, target_node->m_position.z);
}
