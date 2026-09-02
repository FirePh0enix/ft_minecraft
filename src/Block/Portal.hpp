#pragma once

#include "Block/Block.hpp"
#include "Core/Math.hpp"

#include <memory>

class Material;
class BindGroup;

class PortalBlock : public Block
{
public:
    PortalBlock();

private:
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_mat;
    std::shared_ptr<BindGroup> m_bg;
};
