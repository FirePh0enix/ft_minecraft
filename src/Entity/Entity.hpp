#pragma once

#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Option.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Event.hpp"
#include "Transform3D.hpp"

// from `World.hpp`
class World;
class Dimension;
// from `Renderer.hpp`
struct RenderPass;

enum class RpcTarget
{
    Both,
    Client,
    Server,
};

struct EntityId
{
    constexpr EntityId()
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
    ALWAYS_INLINE uint32_t value() const { return m_inner; }

private:
    uint32_t m_inner;
};

template <>
struct std::formatter<EntityId>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end())
            return it;
        return it;
    }

    template <typename FmtContext>
    FmtContext::iterator format(EntityId id, FmtContext& ctx)
    {
        std::ostringstream os;
        os << "EntityId(";
        os << id.value();
        os << ")";
        return std::ranges::copy(std::move(os).str(), ctx.out());
    }
};

class EntitySerializer
{
public:
    template <typename T>
    void set(std::string_view attrib, const T& value)
    {
        m_variants[std::string(attrib)] = Variant(value);
    }

    template <typename T>
    Option<T> get(std::string_view attrib) const
    {
        auto iter = m_variants.find(attrib);
        if (iter != m_variants.end())
            return iter->second.get_unchecked<T>();
        return None;
    }

    template <typename T>
    Option<std::vector<T>> get_array(std::string_view attrib) const
    {
        auto iter = m_variants.find(attrib);
        if (iter != m_variants.end())
            return iter->second.to_array<T>();
        return None;

        // return m_variants.get(attrib).template map<std::vector<T>>([](const Variant& v) -> std::vector<T>
        //                                                            { return v.to_array<T>(); });
    }

    Result<void> save(std::string_view path) const;
    Result<void> load(std::string_view path);

private:
    stdext::string_map<Variant> m_variants;
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

    virtual void draw(const RenderPass& pass)
    {
        (void)pass;
    }

    virtual void draw_ui(const RenderPass& pass)
    {
        (void)pass;
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

    void add_child(std::shared_ptr<Entity> entity);
    void remove_child(size_t index);

    std::shared_ptr<Entity> get_child(size_t index);

    ALWAYS_INLINE const std::vector<std::shared_ptr<Entity>>& get_children() const { return m_children; }

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
    void call_rpc(std::string_view name, Args&&...args)
    {
        std::vector<Variant> variants;
        (void(variants.push_back(std::forward<Args>(args))), ...);
        call_rpci(name, variants);
    }

    void call_rpc(std::string_view name) { call_rpci(name, {}); }

    const AABB& get_aabb() const { return m_aabb; }

    void recurse_tick(float delta);

    ALWAYS_INLINE bool is_active() const { return m_active; }

    Option<RpcTarget> get_rpc(std::string_view name);

    void move_and_collide();
    bool is_in_water() const;
    bool chunk_is_loaded() const;

protected:
    EntityId m_id;
    Entity *m_parent = nullptr; // FIXME: This must be changed by either a std::shared_ptr<Entity> or a EntityId.
    std::vector<std::shared_ptr<Entity>> m_children;
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

    template <typename T>
    static void expose_rpc(std::string_view name, RpcTarget target = RpcTarget::Both) { expose_rpc(T::get_static_hash_code(), name, target); }
    static void expose_rpc(ClassHashCode cls, std::string_view name, RpcTarget target = RpcTarget::Both);

    void call_rpci(std::string_view name, std::span<const Variant> args);
};
