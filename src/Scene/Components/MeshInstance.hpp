#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"
#include "Scene/Components/Visual.hpp"

class MeshInstance : public VisualComponent
{
    CLASS(MeshInstance, VisualComponent);

public:
    MeshInstance(const Ref<Mesh>& mesh, const Ref<Material>& material)
        : m_mesh(mesh), m_material(material)
    {
    }

    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) override
    {
        graph.add_draw(m_mesh.ptr(), m_material.ptr(), camera.get_view_proj_matrix());
    }

private:
    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
};
