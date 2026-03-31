#include "Pathfinding.hpp"
#include "Core/Print.hpp"

Node *Pathfinding::node_from_world_point(const glm::vec3& pos)
{
    glm::ivec3 key{int(round(pos.x)), int(round(pos.y)), int(round(pos.z))};
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

std::vector<Node *> Pathfinding::get_neighbors(const Node& node)
{
    std::vector<Node *> neighbors;

    static const std::vector<glm::ivec3> directions = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    for (const glm::ivec3& dir : directions)
    {
        glm::ivec3 neighbor_pos = node.m_gridPos + dir;

        if (!m_world->get_block_state(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z).is_air())
            continue;

        Node *neighbor_node = nullptr;
        auto it = m_nodes.find(neighbor_pos);
        if (it != m_nodes.end())
            neighbor_node = it->second;
        else
        {
            neighbor_node = new Node(true, glm::vec3(neighbor_pos), neighbor_pos);
            m_nodes[neighbor_pos] = neighbor_node;
        }

        neighbors.push_back(neighbor_node);
    }

    return neighbors;
};

int Pathfinding::get_distance(const Node& node_a, const Node& node_b)
{
    int dx = std::abs(node_a.m_gridPos.x - node_b.m_gridPos.x);
    int dz = std::abs(node_a.m_gridPos.z - node_b.m_gridPos.z);
    int dy = std::abs(node_a.m_gridPos.y - node_b.m_gridPos.y);

    int horizontal = (dx > dz) ? 14 * dz + 10 * (dx - dz) : 14 * dx + 10 * (dz - dx);

    return horizontal + 10 * dy;
}

void Pathfinding::retrace_path(Node *start_node, Node *end_node)
{
    std::vector<Node *> path;
    Node *current_node = end_node;

    while (current_node != nullptr && current_node != start_node)
    {
        path.push_back(current_node);
        current_node = current_node->m_parent;
    }

    if (current_node == start_node)
        path.push_back(start_node);

    std::reverse(path.begin(), path.end());

    m_path = path;
}

void Pathfinding::find_path(const glm::vec3& start_pos, const glm::vec3& target_pos)
{
    m_open_set.clear();
    m_close_set.clear();
    m_nodes.clear();

    Node *start_node = node_from_world_point(start_pos);
    Node *target_node = node_from_world_point(target_pos);

    println("start_pos: {} {} {}", start_pos.x, start_pos.y, start_pos.z);
    println("target_pos: {} {} {}", target_pos.x, target_pos.y, target_pos.z);

    m_open_set.push_back(start_node);

    int created_node = 0;

    while (!m_open_set.empty() && created_node < 100)
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
        created_node += 1;

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
}