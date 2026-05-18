#include "Pathfinding.hpp"
#include "Core/Print.hpp"
#include "Core/Result.hpp"
#include "Entity/Pathfinding/PathNode.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>

constexpr int straight = 10;
constexpr int diag_xz = 14;
constexpr int vertical = 12;

Result<uint32_t> Pathfinding::node_from_world_point(const glm::vec3& pos)
{
    glm::ivec3 key(std::floor(pos.x), std::floor(pos.y), std::floor(pos.z));

    auto it = m_node_lookup.find(key);
    if (it != m_node_lookup.end())
        return it->second;

    bool walkable = m_world->get_block_state(key.x, key.y, key.z).is_air();
    TRY(m_nodes.emplace(walkable, pos, key));
    uint32_t index = m_nodes.size() - 1;

    m_node_lookup[key] = index;

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

Result<InplaceVector<uint32_t, 10>> Pathfinding::get_neighbors(uint32_t current_index)
{
    InplaceVector<uint32_t, 10> neighbors;
    const PathNode& current = m_nodes.get_unchecked(current_index);

    static InplaceVector<glm::ivec3, 10> directions = {
        {1, 0, 0},
        {-1, 0, 0},
        {0, 0, 1},
        {0, 0, -1},

        {0, 1, 0},
        {0, -1, 0},

        {-1, 0, 1},
        {1, 0, 1},
        {-1, 0, -1},
        {1, 0, -1},

    };

    for (const glm::ivec3& dir : directions)
    {

        glm::ivec3 neighbor_pos = current.m_gridPos + dir;

        // Track air time so A* won't create infinite neighbors and explore only to where Mob can jump.
        int air_time = current.m_air_time;
        bool on_ground = !m_world->get_block_state(current.m_gridPos.x, current.m_gridPos.y - 1, current.m_gridPos.z).is_air();
        if (on_ground)
            air_time = 0;

        int remaining_jump = 1 - air_time;

        if (!is_walkable(neighbor_pos, remaining_jump))
            continue;

        glm::ivec3 d = neighbor_pos - current.m_gridPos;

        // diagonal in XZ.
        if (std::abs(d.x) == 1 && std::abs(d.z) == 1)
        {
            glm::ivec3 side1(current.m_gridPos.x + d.x, current.m_gridPos.y, current.m_gridPos.z);
            glm::ivec3 side2(current.m_gridPos.x, current.m_gridPos.y, current.m_gridPos.z + d.z);

            // if either side cell is occupied, don't cut the corner.
            if (!m_world->get_block_state(side1.x, side1.y, side1.z).is_air() ||
                !m_world->get_block_state(side2.x, side2.y, side2.z).is_air())
                continue;
        }

        uint32_t neighbor_index;
        auto it = m_node_lookup.find(neighbor_pos);
        if (it != m_node_lookup.end())
            neighbor_index = it->second;
        else
        {
            TRY(m_nodes.emplace(true, glm::vec3(neighbor_pos), neighbor_pos));
            neighbor_index = m_nodes.size() - 1;

            PathNode& neighbor_node = m_nodes.get_unchecked(neighbor_index);
            bool neighbor_on_ground = !m_world->get_block_state(neighbor_pos.x, neighbor_pos.y - 1, neighbor_pos.z).is_air();
            neighbor_node.m_air_time = neighbor_on_ground ? 0 : air_time + 1;

            m_node_lookup[neighbor_pos] = neighbor_index;
        }

        neighbors.push_back(neighbor_index);
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

Result<void> Pathfinding::retrace_path(uint32_t start_index, uint32_t end_index)
{
    uint32_t current_index = end_index;

    while (current_index != start_index)
    {
        TRY(m_path.append(current_index));
        size_t parent = m_nodes.get_unchecked(current_index).m_parent;

        if (parent == std::numeric_limits<size_t>::max())
            break;

        current_index = parent;
    }

    TRY(m_path.append(start_index));

    for (size_t i = 0, j = m_path.size() - 1; i < j; ++i, --j)
        std::swap(m_path.get_unchecked(i), m_path.get_unchecked(j));

    println("-- Found Path --");
    for (size_t i = 0; i < m_path.size(); i++)
    {
        uint32_t node_index = m_path.get_unchecked(i);
        const auto pos = m_nodes.get_unchecked(node_index).m_gridPos;

        println("[{} {} {}]", pos.x, pos.y, pos.z);
    }

    return Result<void>();
}

Result<void> Pathfinding::find_path(const glm::vec3& start_pos, const glm::vec3& target_pos)
{
    m_open_set.clear();
    m_close_set.clear();
    m_nodes.clear();
    m_path.clear();
    m_node_lookup.clear();
    m_path_index = 0;

    uint32_t start_index = node_from_world_point(start_pos).value();
    uint32_t target_index = node_from_world_point(target_pos).value();

    PathNode& start_node = m_nodes.get_unchecked(start_index);
    PathNode& target_node = m_nodes.get_unchecked(target_index);

    start_node.m_g_cost = 0;
    start_node.m_h_cost = get_distance(start_node, target_node);

    TRY(m_open_set.append(start_index));

    while (!m_open_set.empty())
    {
        uint32_t current_index = m_open_set.get_unchecked(0);

        for (size_t i = 1; i < m_open_set.size(); ++i)
        {
            uint32_t candidate = m_open_set.get_unchecked(i);

            const PathNode& a = m_nodes.get_unchecked(candidate);
            const PathNode& b = m_nodes.get_unchecked(current_index);

            int a_f = a.get_f_cost();
            int b_f = b.get_f_cost();

            if (a_f < b_f || (a_f == b_f && a.m_h_cost < b.m_h_cost))
                current_index = candidate;
        }

        m_open_set.erase_swap(current_index);
        m_close_set.insert(m_nodes.get_unchecked(current_index).m_gridPos);

        if (m_nodes.get_unchecked(current_index).m_gridPos == m_nodes.get_unchecked(target_index).m_gridPos)
        {
            TRY(retrace_path(start_index, current_index));
            return Result<void>();
        }

        for (uint32_t neighbor_index : get_neighbors(current_index).value())
        {
            PathNode& neighbor = m_nodes.get_unchecked(neighbor_index);
            PathNode& current = m_nodes.get_unchecked(current_index);

            if (!neighbor.m_walkable || m_close_set.contains(neighbor.m_gridPos))
                continue;

            int new_cost = current.m_g_cost + get_distance(current, neighbor);

            if (new_cost < neighbor.m_g_cost || std::find(m_open_set.begin(), m_open_set.end(), neighbor_index) == m_open_set.end())
            {
                neighbor.m_g_cost = new_cost;
                neighbor.m_h_cost = get_distance(neighbor, m_nodes.get_unchecked(target_index));
                neighbor.m_parent = current_index;

                if (std::find(m_open_set.begin(), m_open_set.end(), neighbor_index) == m_open_set.end())
                    TRY(m_open_set.append(neighbor_index));
            }
        }
    }

    println("Cannot find path. Start_pos: [{} {} {}], target_pos: [{} {} {}]", start_pos.x, start_pos.y, start_pos.z, target_pos.x, target_pos.y, target_pos.z);
    return Result<void>();
}

Result<Vector<glm::vec3>> Pathfinding::simplify_path(const LocalVector<uint32_t>& path)
{
    Vector<glm::vec3> waypoints;

    TRY(waypoints.append(m_nodes.get_unchecked(path.get_unchecked(0)).m_position));

    if (path.size() < 2)
        return waypoints;

    glm::ivec3 last_direction =
        m_nodes.get_unchecked(path.get_unchecked(1)).m_gridPos -
        m_nodes.get_unchecked(path.get_unchecked(0)).m_gridPos;

    for (size_t i = 2; i < path.size(); i++)
    {
        glm::ivec3 current_direction =
            m_nodes.get_unchecked(path.get_unchecked(i)).m_gridPos -
            m_nodes.get_unchecked(path.get_unchecked(i - 1)).m_gridPos;

        if (current_direction != last_direction)
        {
            TRY(waypoints.append(
                m_nodes.get_unchecked(path.get_unchecked(i - 1)).m_position));

            last_direction = current_direction;
        }
    }

    TRY(waypoints.append(m_nodes.get_unchecked(path.get_unchecked(path.size() - 1)).m_position));

    println("-- Simplify Path --");
    for (size_t i = 0; i < waypoints.size(); i++)
    {
        const auto pos = waypoints.get_unchecked(i);
        println("[{} {} {}]", pos.x, pos.y, pos.z);
    }

    return waypoints;
}