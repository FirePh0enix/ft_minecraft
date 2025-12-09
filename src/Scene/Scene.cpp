#include "Scene/Scene.hpp"

#include <ranges>

Scene::Scene()
{
}

void Scene::tick(float delta)
{
    m_physics_space.step(delta);
}

Ref<Entity> Scene::get_entity(const EntityPath& path) const
{
    std::vector<std::string_view> parts;

    for (auto view : path.path() | std::views::split('/') | std::views::transform([](auto v)
                                                                                  { return std::string_view(v.data(), v.size()); }))
        parts.push_back(view);

    size_t index = 0;
    Ref<Entity> curr_entity;
    std::vector<Ref<Entity>> entities = m_entities;

    do
    {
        for (const Ref<Entity>& entity : entities)
        {
            if (entity->get_name() == parts[index])
            {
                curr_entity = entity;
                entities = curr_entity->get_children();
                break;
            }
        }
    } while (index < parts.size());

    return index == parts.size() ? curr_entity : nullptr;
}

std::vector<QueryResultInternal> Scene::query_internal(const std::vector<Ref<Entity>>& entities, const QueryResultMeta& meta)
{
    std::vector<QueryResultInternal> results;

    std::vector<Ref<Component>> components;
    components.reserve(meta.included_classes.size());

    for (Ref<Entity> entity : entities)
    {
        std::vector<QueryResultInternal> sub_results = query_internal(entity->get_children(), meta);

        // First check if any component excluded by a `Not<...>` is present.
        bool has_excluded = false;
        for (ClassHashCode class_hash : meta.excluded_classes)
        {
            if (entity->has_component(class_hash))
            {
                has_excluded = true;
                break;
            }
        }

        if (has_excluded)
            continue;

        for (ClassHashCode class_hash : meta.included_classes)
        {
            Ref<Component> comp = entity->get_component(class_hash);

            if (comp.is_null())
                break;
            components.push_back(comp);
        }

        if (components.size() == meta.included_classes.size())
        {
            std::vector<QueryResultInternal> child_results;
            for (const QueryResultMeta& child : meta.children)
            {
                std::vector<QueryResultInternal> child_res = query_internal(entity->get_children(), child);
                child_results.insert(results.end(), child_res.begin(), child_res.end());
            }
            results.push_back(QueryResultInternal(std::move(components), std::move(child_results)));
        }

        results.insert(results.end(), sub_results.begin(), sub_results.end());

        components.resize(0);
    }
    return results;
}

#ifdef __has_debug_menu
#include <imgui.h>

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
                bool v = property.getter(m_selected_component.ptr());
                if (ImGui::Checkbox(name.c_str(), &v))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            case PrimitiveType::Float:
            {
                float v = property.getter(m_selected_component.ptr());
                if (ImGui::InputFloat(name.c_str(), &v))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            case PrimitiveType::Vec2:
            {
                glm::vec2 v = property.getter(m_selected_component.ptr());
                if (ImGui::InputFloat2(name.c_str(), &v[0]))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            case PrimitiveType::Vec3:
            {
                glm::vec3 v = property.getter(m_selected_component.ptr());
                if (ImGui::InputFloat3(name.c_str(), &v[0]))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            case PrimitiveType::Vec4:
            {
                glm::vec4 v = property.getter(m_selected_component.ptr());
                if (ImGui::InputFloat4(name.c_str(), &v[0]))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            case PrimitiveType::Quat:
            {
                glm::quat v = property.getter(m_selected_component.ptr());
                if (ImGui::InputFloat4(name.c_str(), &v[0]))
                    property.setter(m_selected_component.ptr(), Variant(v));
            }
            break;
            }
        }
    }
    ImGui::End();
}

void Scene::imgui_debug_with_entity(const Ref<Entity>& entity, size_t index)
{
    (void)index;
    char buffer[128];
    sprintf(buffer, "%s", entity->get_name().c_str());

    if (ImGui::TreeNodeEx(buffer, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesToNodes))
    {
        for (const auto& comp : entity->get_components())
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
