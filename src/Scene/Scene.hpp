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

    void encode_draw_calls(RenderPassEncoder& encoder);

    void tick(float delta);

    /**
     * Add a plugin to the scene.
     * A plugin is a type which has a single `static void setup(Scene *scene)` function which add system and entities.
     */
    template <typename T>
        requires IsPlugin<T>
    void add_plugin()
    {
        T::setup(this);
    }

    /**
     * Add multiple plugins at once, see `Scene::add_plugin` for more informations.
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
                      "Callable must have signature void(const Query<...>&)");

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
                                      internal.collections.push_back(QueryCollectionInternal(query_internal(m_entities, result)));
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
     * Find an entity from a path.
     */
    Ref<Entity> get_entity(const EntityPath& path) const;

    inline Ref<Camera>& get_active_camera()
    {
        return m_active_camera;
    }

    ALWAYS_INLINE void set_active_camera(const Ref<Camera>& camera)
    {
        m_active_camera = camera;
    }

    void add_entity(Ref<Entity>& entity)
    {
        EntityId id = allocate_next_id();

        entity->set_id(id);
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

    std::vector<QueryResultInternal> query_internal(const std::vector<Ref<Entity>>& entities, const QueryResultMeta& meta)
    {
        std::vector<QueryResultInternal> results;

        std::vector<Ref<Component>> components;
        components.reserve(meta.included_classes.size());

        for (Ref<Entity> entity : entities)
        {
            bool has_excluded = false;
            std::vector<QueryResultInternal> sub_results = query_internal(entity->get_children(), meta);

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
};
