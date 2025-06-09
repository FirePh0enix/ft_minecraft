#pragma once

#include "Core/Ref.hpp"
#include "Render/Graph.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Entity.hpp"

class Scene
{
public:
    Scene();

    void encode_draw_calls(RenderGraph& graph);

    void tick();

    inline void set_active_camera(const Ref<Camera>& camera)
    {
        m_active_camera = camera;
    }

    inline void add_entity(Ref<Entity>& entity)
    {
        m_entites.push_back(entity);
        entity->set_id(allocate_next_id());
        entity->start();
    }

    inline std::vector<Ref<Entity>>& get_entities()
    {
        return m_entites;
    }

private:
    Ref<Camera> m_active_camera;
    std::vector<Ref<Entity>> m_entites;
    EntityId m_id = EntityId(1);

    EntityId allocate_next_id()
    {
        EntityId id = m_id;
        m_id = EntityId((uint32_t)m_id + 1);
        return id;
    }
};
