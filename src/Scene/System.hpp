#pragma once

#include "Core/Ref.hpp"
#include "Scene/Components/Component.hpp"

class Scene;

enum Lifecycle
{
    EarlyStartup,
    Startup,
    LateStartup,

    EarlyUpdate,
    Update,
    LateUpdate,

    EarlyFixedUpdate,
    FixedUpdate,
    LateFixedUpdate,

    LifeCycleSize,
};

template <typename T, typename... Ts>
struct Index;

template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0>
{
};

template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value>
{
};

struct QueryResultInternal
{
    QueryResultInternal(const std::vector<Ref<Component>>&& components, const std::vector<QueryResultInternal>&& child_results)
        : components(components), child_results(child_results)
    {
    }

protected:
    std::vector<Ref<Component>> components;
    std::vector<QueryResultInternal> child_results;
};

struct QueryInternal
{
    QueryInternal(Scene *scene, const std::vector<QueryResultInternal>&& results)
        : scene(scene), results_internal(results)
    {
    }

protected:
    Scene *scene;
    std::vector<QueryResultInternal> results_internal;
};

template <typename... T>
struct QueryResult : public QueryResultInternal
{
    explicit QueryResult(QueryResultInternal internal)
        : QueryResultInternal(internal)
    {
    }

    template <typename B>
    Ref<B> get() const
    {
        return components[Index<B, T...>::value].template cast_to<B>();
    }

    template <typename... B>
    const std::vector<QueryResult<B...>>& children() const
    {
        return *(std::vector<QueryResult<B...>> *)&child_results;
    }
};

template <typename... T>
struct Query : public QueryInternal
{
    QueryResult<T...> single() const
    {
        return QueryResult<T...>(results_internal[0]);
    }

    const std::vector<QueryResult<T...>>& results() const
    {
        return *(std::vector<QueryResult<T...>> *)&results_internal;
    }
};

using SystemInternalFunc = void (*)(const QueryInternal& query);

template <typename... T>
using SystemFunc = void (*)(const Query<T...>& query);

/**
 * Exclude a component during a system query. Use it like `Query<Transformed3D, Not<RigidBody>>` which will query all
 * entities except those which have a `RigidBody` component.
 *
 * /!\ Must be specified at the END of the query parameters /!\
 */
template <typename T>
struct Not
{
};

/**
 * Query children of the main query.
 *
 * /!\ Must be specified at the END of the query parameters /!\
 */
template <typename... Ts>
struct Child
{
};

template <typename T>
struct Tag
{
    using Type = T;
};

struct SystemMap
{
    struct EntryClasses
    {
        std::vector<ClassHashCode> included;
        std::vector<ClassHashCode> excluded;
        std::vector<EntryClasses> children;
    };

    struct Entry
    {
        SystemInternalFunc func;
        EntryClasses classes;
    };

    template <typename... Ts>
    void add_system(Lifecycle lifecycle, std::type_identity_t<SystemFunc<Ts...>> system)
    {
        std::vector<ClassHashCode> classes;
        std::vector<ClassHashCode> excluded_classes;
        std::vector<EntryClasses> children;

        place_in_correct_array(Tag<Child<Ts...>>{}, classes, excluded_classes, children);

        m_systems[lifecycle].push_back(Entry{.func = (SystemInternalFunc)(void *)system, .classes = children[0]});
    }

    void for_each(Lifecycle lifecycle, std::function<void(const Entry& system)> f)
    {
        for (const Entry& system : m_systems[lifecycle])
            f(system);
    }

private:
    std::vector<Entry> m_systems[LifeCycleSize];

    template <typename T>
    void place_in_correct_array(Tag<T>, std::vector<ClassHashCode>& classes, std::vector<ClassHashCode>&, std::vector<EntryClasses>&)
    {
        classes.push_back(T::get_static_hash_code());
    }

    template <typename... Ts>
    void place_in_correct_array(Tag<Child<Ts...>>, std::vector<ClassHashCode>&, std::vector<ClassHashCode>&, std::vector<EntryClasses>& children)
    {
        std::vector<ClassHashCode> classes2;
        std::vector<ClassHashCode> excluded_classes2;
        std::vector<EntryClasses> children2;
        (place_in_correct_array(Tag<Ts>{}, classes2, excluded_classes2, children2), ...);

        children.push_back(EntryClasses(classes2, excluded_classes2, children2));
    }

    template <typename T>
    void place_in_correct_array(Tag<Not<T>>, std::vector<ClassHashCode>&, std::vector<ClassHashCode>& excluded_classes, std::vector<EntryClasses>&)
    {
        excluded_classes.push_back(T::get_static_hash_code());
    }
};
