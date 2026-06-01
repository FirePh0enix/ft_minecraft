#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Definitions.hpp"
#include "Core/Ref.hpp"
#include "Core/String.hpp"
#include "Event.hpp"
#include "Render/Renderer.hpp"
#include "Transform3D.hpp"

class World;
class Dimension;

enum class RpcTarget
{
    Both,
    Client,
    Server,
};

struct EntityId
{
    EntityId()
        : m_inner(0)
    {
    }

    explicit EntityId(uint32_t id)
        : m_inner(id)
    {
    }

    ALWAYS_INLINE operator uint32_t() const
    {
        return m_inner;
    }

    ALWAYS_INLINE bool operator==(const EntityId& b) const
    {
        return m_inner == b.m_inner;
    }

    ALWAYS_INLINE std::strong_ordering operator<=>(const EntityId& b) const
    {
        return m_inner <=> b.m_inner;
    }

    ALWAYS_INLINE bool is_valid() const { return m_inner != 0; }

private:
    uint32_t m_inner;
};

class EntitySerializer
{
public:
    template <typename T>
    Result<void> set(const StringView& attrib, const T& value)
    {
        TRY(m_variants.put(attrib, Variant(value)));
        return Result<void>();
    }

    template <typename T>
    Option<T> get(const StringView& attrib) const
    {
        return m_variants.get(attrib).template map<T>([](const Variant& v) -> T
                                                      { return v.get_unchecked<T>(); });
    }

    template <typename T>
    Option<Vector<T>> get_array(const StringView& attrib) const
    {
        return m_variants.get(attrib).template map<Vector<T>>([](const Variant& v) -> Vector<T>
                                                              { return EXPECT(v.to_array<T>()); });
    }

    Result<void> save(const StringView& path) const;
    Result<void> load(const StringView& path);

private:
    HashMap<String, Variant> m_variants;
};

class Entity : public Object
{
    CLASS(Entity, Object);

    friend class World;

public:
    Entity();
    virtual ~Entity() {}

    static void bind_methods();

    virtual void start()
    {
    }

    virtual void tick(float delta)
    {
        (void)delta;
    }

    virtual void draw(const RenderPassNode& node)
    {
        (void)node;
    }

    virtual void draw_ui(const RenderPassNode& node)
    {
        (void)node;
    }

    virtual void on_ready()
    {
    }

    virtual void process_event(Event& event)
    {
        (void)event;
    }

    /**
     * Called when saving the entity to disk.
     */
    virtual void save(EntitySerializer& ser) const
    {
        (void)ser;
    }

    /**
     * Called when loading the entity from the disk.
     */
    virtual void load(const EntitySerializer& deser)
    {
        (void)deser;
    }

    const Transform3D& get_transform() const { return m_transform; }
    Transform3D& get_transform() { return m_transform; }

    Transform3D get_global_transform() const;

    void add_child(Ref<Entity> entity);
    void remove_child(size_t index);

    Ref<Entity> get_child(size_t index);

    ALWAYS_INLINE View<Ref<Entity>> get_children() const { return m_children; }

    ALWAYS_INLINE Entity *get_parent() const { return m_parent; }
    ALWAYS_INLINE void set_parent(Entity *parent) { m_parent = parent; }
    ALWAYS_INLINE bool has_parent() const { return m_parent != nullptr; }

    ALWAYS_INLINE void set_id(EntityId id) { m_id = id; }
    ALWAYS_INLINE EntityId id() const { return m_id; }

    void set_position(glm::vec3 position) { get_transform().position() = position; }
    glm::vec3 get_position() const { return get_transform().position(); }

    void set_rotation(glm::quat rotation) { get_transform().rotation() = rotation; }
    glm::quat get_rotation() const { return get_transform().rotation(); }

    template <typename... Args>
    void call_rpc(const StringView& name, Args&&...args)
    {
        InplaceVector<Variant, sizeof...(args)> variants;
        (variants.append(args), ...);
        call_rpc(name, variants);
    }

    void call_rpc(const StringView& name) { call_rpc(name, {}); }

    const AABB& get_aabb() const { return m_aabb; }

    void recurse_tick(float delta);

    ALWAYS_INLINE bool is_active() const { return m_active; }

    Option<RpcTarget> get_rpc(StringView name);

    void move_and_collide();
    bool is_in_water() const;
    bool chunk_is_loaded() const;

    static void cleanup()
    {
        s_exposed_rpc.clear();
    }

protected:
    EntityId m_id;
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a Ref<Entity> or a EntityId.
    LocalVector<Ref<Entity>> m_children;
    Transform3D m_transform;
    AABB m_aabb;

    float m_gravity_value = 9.81 / 10.0;
    bool m_on_ground = false;
    glm::vec3 m_velocity = glm::vec3();

    World *m_world = nullptr;

    bool m_active = true;
    size_t m_dimension = 0;

    bool m_blocked_x = false;
    bool m_blocked_z = false;

    static inline HashMap<ClassHashCode, HashMap<String, RpcTarget>> s_exposed_rpc;

    template <typename T>
    static ALWAYS_INLINE void expose_rpc(String name, RpcTarget target = RpcTarget::Both)
    {
        if (!s_exposed_rpc.contains(T::get_static_hash_code()))
            EXPECT(s_exposed_rpc.put(T::get_static_hash_code(), {}));
        EXPECT(s_exposed_rpc.get_ptr(T::get_static_hash_code()).get()->put(name, target));
    }

    void call_rpc(StringView name, View<Variant> args);
};
