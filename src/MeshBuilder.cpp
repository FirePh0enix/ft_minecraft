#include "MeshBuilder.hpp"

#include "Render/Renderer.hpp"
#include <span>

Result<std::shared_ptr<Mesh>> MeshBuilder::build()
{
    return Mesh::create_from_data(std::as_bytes(std::span(m_indices)), m_vertices, m_normals, std::as_bytes(std::span(m_uvs)));
}
