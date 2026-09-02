#include "Block/PlantBlock.hpp"

#include "Engine.hpp"
#include "Render/Renderer.hpp"

PlantBlock::PlantBlock(std::string_view texture)
{
    m_conventional = false;
    m_transparent = true;
    m_texture = Engine::get().registry().load_texture(texture);
}

void PlantBlock::add_to_mesh(glm::i64vec3 position, std::vector<uint16_t>& indices, std::vector<glm::vec3>& vertices, std::vector<glm::vec4>& uvs, std::vector<glm::vec3>& normals)
{
    uint16_t i0 = vertices.size() + 0;
    uint16_t i1 = vertices.size() + 1;
    uint16_t i2 = vertices.size() + 2;
    uint16_t i3 = vertices.size() + 3;

    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);

    indices.push_back(i2);
    indices.push_back(i3);
    indices.push_back(i0);

    const glm::vec3 offset = glm::vec3(position.x, position.y, position.z) - glm::vec3(0.5);
    vertices.push_back(glm::vec3(0.0, 0.0, 0.0) + offset);
    vertices.push_back(glm::vec3(0.0, 1.0, 0.0) + offset);
    vertices.push_back(glm::vec3(1.0, 1.0, 0.0) + offset);
    vertices.push_back(glm::vec3(1.0, 0.0, 0.0) + offset);

    uvs.push_back(glm::vec4(0.0, 0.0, (float)m_texture, 0.0));
    uvs.push_back(glm::vec4(1.0, 0.0, (float)m_texture, 0.0));
    uvs.push_back(glm::vec4(1.0, 1.0, (float)m_texture, 0.0));
    uvs.push_back(glm::vec4(0.0, 1.0, (float)m_texture, 0.0));

    const glm::vec3 normal = glm::vec3(1.0, 0.0, 0.0); // TODO
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
    normals.push_back(normal);
}
