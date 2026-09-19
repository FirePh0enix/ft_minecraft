#include "MeshBuilder.hpp"

#include "Render/Renderer.hpp"
#include <span>

std::expected<std::shared_ptr<Mesh>, Error> MeshBuilder::build()
{
    return Mesh::create_from_data(std::as_bytes(std::span(m_indices)), m_vertices, m_normals, std::as_bytes(std::span(m_uvs)));
}

std::expected<std::shared_ptr<Mesh>, Error> MeshBuilder::build_positions()
{
    return Mesh::create_from_data(std::as_bytes(std::span(m_indices)), m_vertices, std::span<glm::vec3>(), std::span<std::byte>());
}

void MeshBuilder::append_positions(const MeshBuilder& other, glm::vec3 offset)
{
    // Source indices start at zero; rebase them into the combined vertex array.
    // The offset converts slice-local positions into chunk-local positions.
    const uint32_t first_vertex = static_cast<uint32_t>(m_vertices.size());
    for (uint32_t index : other.m_indices)
        m_indices.push_back(first_vertex + index);
    for (glm::vec3 vertex : other.m_vertices)
        m_vertices.push_back(vertex + offset);
}
