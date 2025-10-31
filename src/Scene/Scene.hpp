#pragma once

#include "Core/Ref.hpp"
#include "Physics/PhysicsSpace.hpp"
#include "Render/Graph.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Entity.hpp"

class Scene : public Object
{
    CLASS(Scene, Object);

public:
    Scene();

    void encode_draw_calls(RenderPassEncoder& encoder);

    void tick(float delta);

    inline Ref<Camera>& get_active_camera()
    {
        return m_active_camera;
    }

    inline void set_active_camera(const Ref<Camera>& camera)
    {
        m_active_camera = camera;
    }

    inline void add_entity(Ref<Entity>& entity)
    {
        m_entites.push_back(entity);

        entity->set_id(allocate_next_id());
        entity->set_scene(this);
        entity->do_start();
    }

    inline std::vector<Ref<Entity>>& get_entities()
    {
        return m_entites;
    }

    const PhysicsSpace& get_physics_space() const
    {
        return m_physics_space;
    }

    PhysicsSpace& get_physics_space()
    {
        return m_physics_space;
    }

    static EntityId allocate_next_id()
    {
        EntityId id = s_id;
        s_id = EntityId((uint32_t)s_id + 1);
        return id;
    }

    static Ref<Scene>& get_active_scene()
    {
        return s_active_scene;
    }

    static void set_active_scene(const Ref<Scene>& scene)
    {
        s_active_scene = scene;
    }

private:
    Ref<Camera> m_active_camera;
    std::vector<Ref<Entity>> m_entites;
    PhysicsSpace m_physics_space;

    static inline EntityId s_id = EntityId(1);
    static inline Ref<Scene> s_active_scene;
};
