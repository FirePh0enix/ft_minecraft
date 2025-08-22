#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"
#include "Scene/Components/Visual.hpp"

struct StandardMeshPushConstants
{
    glm::mat4 view_matrix;
    glm::mat4 model_matrix;
};

struct StandardMeshMaterialInfo
{
    glm::vec3 base_color = glm::vec3(1.0, 1.0, 1.0);
};

class MeshInstance : public VisualComponent
{
    CLASS(MeshInstance, VisualComponent);

public:
    MeshInstance(const Ref<Mesh>& mesh, const Ref<Material>& material)
        : m_mesh(mesh), m_material(material), m_visible(true)
    {
        m_material_buffer = RenderingDriver::get()->create_buffer(sizeof(StandardMeshMaterialInfo), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Uniform).value();
        m_material->set_param("material", m_material_buffer);
    }

    virtual void start() override
    {
        m_transform = m_entity->get_transform();
    }

    inline Ref<Material>& get_material()
    {
        return m_material;
    }

    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) override
    {
        if (is_visible())
        {
            graph.bind_material(m_material);

            graph.bind_index_buffer(m_mesh->get_buffer(MeshBufferKind::Index));
            graph.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Position), 0);
            graph.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Normal), 1);
            graph.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::UV), 2);

            StandardMeshPushConstants push_constants{};
            push_constants.view_matrix = camera.get_view_proj_matrix();
            push_constants.model_matrix = m_transform->get_global_transform().to_matrix();
            std::vector<char> bytes(sizeof(StandardMeshPushConstants));
            std::memcpy(bytes.data(), (char *)&push_constants, sizeof(StandardMeshPushConstants));
            graph.push_constants(bytes);

            graph.draw(m_mesh->vertex_count(), 1);
        }
    }

    inline bool is_visible() const
    {
        return m_visible;
    }

    inline void set_visible(bool visible)
    {
        m_visible = visible;
    }

    void set_base_color(glm::vec3 base_color)
    {
        m_material_info.base_color = base_color;
        update_buffer();
    }

private:
    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
    Ref<Transformed3D> m_transform;
    bool m_visible;
    Ref<Buffer> m_material_buffer;
    StandardMeshMaterialInfo m_material_info{};

    void update_buffer()
    {
        RenderingDriver::get()->update_buffer(m_material_buffer, View(&m_material_info).as_bytes(), 0);
    }
};
