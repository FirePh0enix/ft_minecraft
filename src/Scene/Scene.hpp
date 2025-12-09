#pragma once

#include "Core/Ref.hpp"
#include "Physics/PhysicsSpace.hpp"
#include "Render/Graph.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Entity.hpp"
#include "Scene/System.hpp"

class Scene;

template <typename T>
concept IsPlugin = requires(Scene *scene) {
    { T::setup(scene) } -> std::same_as<void>;
};

class Scene : public Object
{
    CLASS(Scene, Object);

public:
    Scene();

    void tick(float delta);

    /**
     *  Add a plugin to the scene.
     *  A plugin is a type which has a single `static void setup(Scene *scene)` function which add system and entities.
     */
    template <typename T>
        requires IsPlugin<T>
    void add_plugin()
    {
        T::setup(this);
    }

    /**
     *  Add multiple plugins at once, see `Scene::add_plugin` for more informations.
     */
    template <typename... T>
        requires(IsPlugin<T>, ...)
    void add_plugins()
    {
        (T::setup(this), ...);
    }

    template <class F>
    void add_system(Lifecycle lifecycle, F&& f)
    {
        using PtrType = decltype(+std::forward<F>(f));
        using Traits = SystemTraits<PtrType>;

        static_assert(std::is_same_v<PtrType, typename Traits::Pointer>,
                      "Callable must have signature void(const Query<...>&, Action&)");

        PtrType sys = +std::forward<F>(f);
        m_system_map.add_system(lifecycle, sys);
    }

    void run_systems(Lifecycle lifecycle)
    {
        Action action;
        m_system_map.for_each(lifecycle, [this, &action](const auto& meta)
                              {
                                  QueryInternal internal;
                                  internal.scene = this;

                                  for (auto& result : meta.results)
                                  {
                                      QueryCollectionInternal collection(query_internal(m_entities, result));
                                      internal.collections.push_back(collection);
                                  }

                                  meta.func(internal, action); });
        flush_actions(action);
    }

    void flush_actions(const Action& action)
    {
        for (EntityId id : action.get_entities_to_remove())
            remove_entity(id);

        for (Ref<Entity> entity : action.get_entities_to_add())
            add_entity(entity);
    }

    /**
     *  @brief Find an entity from a path.
     */
    Ref<Entity> get_entity(const EntityPath& path) const;

    ALWAYS_INLINE Ref<Camera>& get_active_camera() { return m_active_camera; }
    ALWAYS_INLINE void set_active_camera(const Ref<Camera>& camera) { m_active_camera = camera; }

    void add_entity(Ref<Entity>& entity)
    {
        EntityId id = allocate_next_id();

        entity->m_id = id;
        entity->set_scene(this);
        entity->do_start();

        if (entity->get_name().empty())
            entity->set_name(format("Entity #{}", (uint32_t)id));

        m_entities.push_back(entity);
    }

    void remove_entity(EntityId id)
    {
        m_entities.erase(std::find(m_entities.begin(), m_entities.end(), id));
    }

    inline std::vector<Ref<Entity>>& get_entities()
    {
        return m_entities;
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

#ifdef __has_debug_menu
    void imgui_debug_window();
    void imgui_debug_with_entity(const Ref<Entity>& entity, size_t index);
#endif

private:
    Ref<Camera> m_active_camera;
    std::vector<Ref<Entity>> m_entities;
    PhysicsSpace m_physics_space;

    SystemMap m_system_map;

#ifdef __has_debug_menu
    Ref<Component> m_selected_component;
#endif

    static inline EntityId s_id = EntityId(1);
    static inline Ref<Scene> s_active_scene;

    std::vector<QueryResultInternal> query_internal(const std::vector<Ref<Entity>>& entities, const QueryResultMeta& meta);
};
