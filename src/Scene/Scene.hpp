#pragma once

#include "Camera.hpp"
#include "Core/Ref.hpp"
#include "Render/Graph.hpp"
#include "Scene/Entity.hpp"

class Scene
{
public:
    Scene();

    void encode_draw_calls(RenderGraph& graph);

    inline void set_active_camera(const Ref<Camera>& camera)
    {
        m_active_camera = camera;
    }

    inline void add_entity(const Ref<Entity>& entity)
    {
        m_entites.push_back(entity);
    }

    inline std::vector<Ref<Entity>>& get_entities()
    {
        return m_entites;
    }

private:
    Ref<Camera> m_active_camera;
    std::vector<Ref<Entity>> m_entites;
};
