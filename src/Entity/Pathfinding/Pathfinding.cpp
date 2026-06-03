#include "Pathfinding.hpp"
#include "Core/Containers/InplaceVector.hpp"
#include "Core/Result.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include <algorithm>
#include <cstddef>

constexpr int straight = 10;
constexpr int diag_xz = 14;
constexpr int vertical = 12;

size_t Pathfinding::node_from_world_point(const glm::ivec3& pos)
{
    glm::ivec3 key(std::floor(pos.x), std::floor(pos.y), std::floor(pos.z));

    auto it = m_nodes.find(key);
    if (it != m_nodes.end())
        return it->second;

    bool walkable = m_world->get_block_state(pos.x, pos.y, pos.z).is_air();

    PathNode node;
    node.m_gridPos = pos;
    node.m_position = glm::vec3(pos);
    node.m_walkable = walkable;

    EXPECT(m_node_pool.append(node));

    size_t index = m_node_pool.size() - 1;

    m_nodes[key] = index;

    return index;
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

Vector<size_t> Pathfinding::get_neighbors(size_t node_index)
{
    Vector<size_t> neighbors;

    const PathNode& node = m_node_pool.get_unchecked(node_index);

    static InplaceVector<glm::ivec3, 10> directions = {

        {-1, 0, 1},
        {1, 0, 1},
        {-1, 0, -1},
        {1, 0, -1},

        {1, 0, 0},
        {-1, 0, 0},
        {0, 0, 1},
        {0, 0, -1},

        {0, 1, 0},
        {0, -1, 0},
    };

    for (const glm::ivec3& dir : directions)
    {
        glm::ivec3 neighbor_pos = node.m_gridPos + dir;

        int air_time = node.m_air_time;

        bool on_ground = !m_world->get_block_state(node.m_gridPos.x, node.m_gridPos.y - 1, node.m_gridPos.z).is_air();

        if (on_ground)
            air_time = 0;

        if (!is_walkable(neighbor_pos, air_time))
            continue;

        glm::ivec3 d = neighbor_pos - node.m_gridPos;

        // diagonal in XZ
        if (std::abs(d.x) == 1 && std::abs(d.z) == 1)
        {
            glm::ivec3 side1(node.m_gridPos.x + d.x, node.m_gridPos.y, node.m_gridPos.z);
            glm::ivec3 side2(node.m_gridPos.x, node.m_gridPos.y, node.m_gridPos.z + d.z);

            if (!m_world->get_block_state(side1.x, side1.y, side1.z).is_air() ||
                !m_world->get_block_state(side2.x, side2.y, side2.z).is_air())
                continue;
        }

        size_t neighbor_index = node_from_world_point(neighbor_pos);

        EXPECT(neighbors.append(neighbor_index));
    }

    return neighbors;
}

int Pathfinding::get_distance(const PathNode& a, const PathNode& b)
{
    int dx = std::abs(a.m_gridPos.x - b.m_gridPos.x);
    int dz = std::abs(a.m_gridPos.z - b.m_gridPos.z);
    int dy = std::abs(a.m_gridPos.y - b.m_gridPos.y);

    int diag_dir = std::min(dx, dz);
    int straight_dir = std::max(dx, dz) - diag_dir;

    int horizontal = diag_xz * diag_dir + straight * straight_dir;
    return horizontal + vertical * dy;
}

void Pathfinding::retrace_path(size_t start_index, size_t end_index)
{
    m_path.clear();

    size_t current = end_index;
    size_t max = std::numeric_limits<size_t>::max();

    while (current != max)
    {
        EXPECT(m_path.append(current));

        if (current == start_index)
            break;

        current = m_node_pool.get_unchecked(current).m_parent;
    }

    // reverse path
    for (size_t i = 0, j = m_path.size() - 1; i < j; ++i, --j)
    {
        auto tmp = m_path.get_unchecked(i);
        m_path.get_unchecked(i) = m_path.get_unchecked(j);
        m_path.get_unchecked(j) = tmp;
    }
}

void Pathfinding::find_path(const glm::vec3& start_pos, const glm::vec3& target_pos)
{
    m_open_set.clear();
    m_close_set.clear();
    m_nodes.clear();
    m_path.clear();

    size_t start_index = node_from_world_point(start_pos);
    size_t target_index = node_from_world_point(target_pos);

    auto& start = m_node_pool.get_unchecked(start_index);
    start.m_g_cost = 0;
    start.m_h_cost = get_distance(start, m_node_pool.get_unchecked(target_index));

    EXPECT(m_open_set.append(start_index));

    while (!m_open_set.empty())
    {
        size_t current_index = m_open_set.get_unchecked(0);
        size_t best_index = 0;

        const auto& target = m_node_pool.get_unchecked(target_index);

        for (size_t i = 1; i < m_open_set.size(); ++i)
        {
            size_t idx = m_open_set.get_unchecked(i);

            const auto& node = m_node_pool.get_unchecked(idx);
            const auto& best = m_node_pool.get_unchecked(current_index);

            if (node.get_f_cost() < best.get_f_cost() ||
                (node.get_f_cost() == best.get_f_cost() &&
                 node.m_h_cost < best.m_h_cost))
            {
                current_index = idx;
                best_index = i;
            }
        }

        EXPECT(m_open_set.remove_at(best_index));

        auto& current = m_node_pool.get_unchecked(current_index);
        m_close_set.insert(current.m_gridPos);

        if (current.m_gridPos == target.m_gridPos)
        {
            retrace_path(start_index, current_index);
            return;
        }

        for (size_t neighbor_index : get_neighbors(current_index))
        {
            auto& neighbor = m_node_pool.get_unchecked(neighbor_index);

            if (!neighbor.m_walkable || m_close_set.contains(neighbor.m_gridPos))
                continue;

            int new_cost = current.m_g_cost + get_distance(current, neighbor);

            if (new_cost < neighbor.m_g_cost || !m_open_set.contains(neighbor_index))
            {
                neighbor.m_g_cost = new_cost;
                neighbor.m_h_cost = get_distance(neighbor, target);
                neighbor.m_parent = current_index;

                if (!m_open_set.contains(neighbor_index))
                    EXPECT(m_open_set.append(neighbor_index));
            }
        }
    }
}

Vector<glm::vec3> Pathfinding::simplify_path(const Vector<size_t>& path)
{
    Vector<glm::vec3> waypoints;

    if (path.empty())
        return waypoints;

    EXPECT(waypoints.append(m_node_pool.get_unchecked(path.get_unchecked(0)).m_position));

    if (path.size() < 2)
        return waypoints;

    glm::ivec3 last_dir = m_node_pool.get_unchecked(path.get_unchecked(1)).m_gridPos - m_node_pool.get_unchecked(path.get_unchecked(0)).m_gridPos;

    for (size_t i = 2; i < path.size(); i++)
    {
        glm::ivec3 curr = m_node_pool.get_unchecked(path.get_unchecked(i)).m_gridPos - m_node_pool.get_unchecked(path.get_unchecked(i - 1)).m_gridPos;

        if (curr != last_dir)
        {
            EXPECT(waypoints.append(m_node_pool.get_unchecked(path.get_unchecked(i - 1)).m_position));
            last_dir = curr;
        }
    }

    EXPECT(waypoints.append(m_node_pool.get_unchecked(path.get_unchecked(path.size() - 1)).m_position));

    return waypoints;
}