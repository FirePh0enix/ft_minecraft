#pragma once

#include "Core/Math.hpp"

class Node
{
public:
    bool m_walkable;
    int m_air_time = 0;

    glm::vec3 m_position;
    glm::ivec3 m_gridPos;

    int m_g_cost = 0;
    int m_h_cost = 0;
    Node *m_parent = nullptr;

    Node(bool walkable, const glm::vec3& position, const glm::vec3& grid_pos) : m_walkable(walkable), m_position(position), m_gridPos(grid_pos) {};

    inline int get_f_cost() const { return m_g_cost + m_h_cost; }
};