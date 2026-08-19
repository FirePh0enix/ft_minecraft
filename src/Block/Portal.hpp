#pragma once

#include "Block/Block.hpp"
#include "Core/Math.hpp"

class Material;
class BindGroup;

class PortalBlock : public Block
{
public:
    PortalBlock();

    virtual void draw(const RenderPass& pass, glm::i64vec3 position) override;

private:
    std::shared_ptr<Material> m_mat;
    std::shared_ptr<BindGroup> m_bg;
};
