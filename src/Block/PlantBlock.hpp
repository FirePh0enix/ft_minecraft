#pragma once

#include "Block/Block.hpp"

#include <memory>

class BindGroup;
class Texture;

class PlantBlock : public Block
{
public:
    PlantBlock(std::string_view texture);

    virtual void add_to_mesh(glm::i64vec3 position, std::vector<uint16_t>& indices, std::vector<glm::vec3>& vertices, std::vector<glm::vec4>& uvs, std::vector<glm::vec3>& normals) override;

private:
    std::shared_ptr<BindGroup> m_bg;
    size_t m_texture;
};
