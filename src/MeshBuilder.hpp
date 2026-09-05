#pragma once

#include "Core/Error.hpp"
#include "Core/Math.hpp"

#include <expected>
#include <memory>
#include <vector>

class Mesh;

class MeshBuilder
{
public:
    void add_index(uint32_t i) { m_indices.push_back(i); }
    void add_vertex(glm::vec3 vertex) { m_vertices.push_back(vertex); }
    void add_uv(glm::vec4 uv) { m_uvs.push_back(uv); }
    void add_normal(glm::vec3 n) { m_normals.push_back(n); }

    size_t vertex_count() const { return m_vertices.size(); }

    std::expected<std::shared_ptr<Mesh>, Error> build();

private:
    std::vector<uint32_t> m_indices;
    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec4> m_uvs;
    std::vector<glm::vec3> m_normals;
};
