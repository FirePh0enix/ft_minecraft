#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"
#include "Scene/Components/Visual.hpp"

class MeshInstance : public VisualComponent
{
    CLASS(MeshInstance, VisualComponent);

public:
    MeshInstance(const Ref<Mesh>& mesh, const Ref<Material>& material)
        : m_mesh(mesh), m_material(material), m_visible(true)
    {
    }

    Ref<Material>& get_material()
    {
        return m_material;
    }

    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) override
    {
        if (is_visible())
            graph.add_draw(m_mesh, m_material, camera.get_view_proj_matrix());
    }

    inline bool is_visible() const
    {
        return m_visible;
    }

    inline void set_visible(bool visible)
    {
        m_visible = visible;
    }

private:
    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
    bool m_visible;
};
