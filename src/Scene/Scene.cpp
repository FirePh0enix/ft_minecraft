#include "Scene/Scene.hpp"
#include "Scene/Components/Visual.hpp"

#ifdef __has_debug_menu
#include <imgui.h>
#endif

Scene::Scene()
{
}

void Scene::encode_draw_calls(RenderPassEncoder& encoder)
{
    for (const auto& entity : m_entities)
    {
        if (Ref<VisualComponent> visual_comp = entity->get_component<VisualComponent>())
        {
            visual_comp->encode_draw_calls(encoder, *m_active_camera);
        }
    }
}

void Scene::tick(float delta)
{
    m_physics_space.step(delta);
}

#ifdef __has_debug_menu

void Scene::imgui_debug_window()
{
    if (ImGui::Begin("Scene"))
    {
        if (ImGui::TreeNodeEx("root", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (size_t index = 0; index < m_entities.size(); index++)
                imgui_debug_with_entity(m_entities[index], index);
            ImGui::TreePop();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Inspector") && !m_selected_component.is_null())
    {
        for (const auto& [name, property] : ClassRegistry::get().get_class(m_selected_component->get_class_hash_code()).properties)
        {
            switch (property.type)
            {
            case PrimitiveType::Bool:
            {
                bool v = property.getter(m_selected_component);
                if (ImGui::Checkbox(name.c_str(), &v))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            case PrimitiveType::Float:
            {
                float v = property.getter(m_selected_component);
                if (ImGui::InputFloat(name.c_str(), &v))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            case PrimitiveType::Vec2:
            {
                glm::vec2 v = property.getter(m_selected_component);
                if (ImGui::InputFloat2(name.c_str(), &v[0]))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            case PrimitiveType::Vec3:
            {
                glm::vec3 v = property.getter(m_selected_component);
                if (ImGui::InputFloat3(name.c_str(), &v[0]))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            case PrimitiveType::Vec4:
            {
                glm::vec4 v = property.getter(m_selected_component);
                if (ImGui::InputFloat4(name.c_str(), &v[0]))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            case PrimitiveType::Quat:
            {
                glm::quat v = property.getter(m_selected_component);
                if (ImGui::InputFloat4(name.c_str(), &v[0]))
                    property.setter(m_selected_component, Variant(v));
            }
            break;
            }
        }
    }
    ImGui::End();
}

void Scene::imgui_debug_with_entity(const Ref<Entity>& entity, size_t index)
{
    char buffer[128];
    sprintf(buffer, "Entity #%zu", index);

    if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes))
    {
        for (const Ref<Component>& comp : entity->get_components())
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_DrawLinesToNodes;

            if (m_selected_component == comp)
                flags |= ImGuiTreeNodeFlags_Selected;

            if (ImGui::TreeNodeEx(comp->get_class_name(), flags))
            {
                if (ImGui::IsItemClicked())
                    m_selected_component = comp;
                ImGui::TreePop();
            }
        }

        if (entity->get_children().size() > 0 && ImGui::TreeNode("childrens"))
        {
            for (size_t index = 0; index < entity->get_children().size(); index++)
                imgui_debug_with_entity(entity->get_children()[index], index);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

#endif
