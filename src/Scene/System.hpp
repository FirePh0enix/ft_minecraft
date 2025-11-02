#pragma once

#include "Core/Ref.hpp"
#include "Core/Traits.hpp"
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

/**
 * Query the first entity which respect the constraints.
 */
template <typename... Ts>
struct One
{
};

/**
 * Query all entities which respect the constraints.
 */
template <typename... Ts>
struct Many
{
};

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

struct QueryResultInternal
{
    std::vector<Ref<Component>> components;
    std::vector<QueryResultInternal> children_;
};

template <typename... Ts>
struct QueryResult : QueryResultInternal
{
    template <typename B>
    Ref<B> get() const { return components[IndexOf<B, Ts...>::value].template cast_to<B>(); }

    template <typename... B>
    const std::vector<QueryResult<B...>>& children() const { return *(std::vector<QueryResult<B...>> *)&children_; }
};

struct QueryCollectionInternal
{
    std::vector<QueryResultInternal> results_;
};

template <typename... Ts>
struct QueryCollection : QueryCollectionInternal
{
    const std::vector<QueryResult<Ts...>>& results() const { return *(std::vector<QueryResult<Ts...>> *)&results_; };
    const QueryResult<Ts...>& single() const { return *(QueryResult<Ts...> *)&results_[0]; }
};

struct QueryInternal
{
    Scene *scene;
    std::vector<QueryCollectionInternal> collections;
};

template <typename>
struct RebindQueryCollection;

template <typename... Ts>
struct RebindQueryCollection<Many<Ts...>>
{
    using Type = QueryCollection<Ts...>;
};

template <typename... Ts>
struct RebindQueryCollection<One<Ts...>>
{
    using Type = QueryCollection<Ts...>;
};

/**
 * In a system, the Query arguments tell the engine which entities to retrieve. It use template arguments to construct
 * complex conditions.
 *
 * To query all RigidBody present in the scene, the argument would look like `Query<Many<RigidBody>>` where `Many`
 * indicates to query all entities matching the condition. Multiple components can be specified like
 * `Query<Many<Transformed3D, RigidBody>>` which will retrieve both the RigidBody component and their transform.
 * `Not<...>` can be use to exclude a component, in that case `Query<Many<Transformed3D, Not<RigidBody>>` will query all
 * entities with a transform that does not have a `RigidBody` attached to them.
 */
template <typename... Ts>
struct Query : QueryInternal
{
    template <const size_t Index>
    using DeduceQueryCollection = RebindQueryCollection<std::tuple_element_t<Index, std::tuple<Ts...>>>::Type;

    template <const size_t Index>
    const DeduceQueryCollection<Index>& get() const
    {
        return *(DeduceQueryCollection<Index> *)&collections[Index];
    }

    Scene *scene() const
    {
        return QueryInternal::scene;
    }
};

using SystemInternalFunc = void (*)(const QueryInternal& query);

template <typename... T>
using SystemFunc = void (*)(const Query<T...>& query);

enum class QueryKind
{
    One,
    Many,
};

struct QueryResultMeta
{
    QueryKind kind = QueryKind::One;
    std::vector<ClassHashCode> included_classes;
    std::vector<ClassHashCode> excluded_classes;
    std::vector<QueryResultMeta> children;
};

struct QueryMeta
{
    SystemInternalFunc func;
    std::vector<QueryResultMeta> results;
};

struct SystemMap
{
    template <typename... Ts>
    void add_system(Lifecycle lifecycle, SystemFunc<Ts...> system)
    {
        std::vector<QueryResultMeta> results;
        (process_one_many(Tag<Ts>{}, results), ...);

        m_systems[lifecycle].push_back(QueryMeta{.func = (SystemInternalFunc)(void *)system, .results = results});
    }

    void for_each(Lifecycle lifecycle, std::function<void(const QueryMeta&)> f)
    {
        for (const QueryMeta& system : m_systems[lifecycle])
            f(system);
    }

private:
    std::vector<QueryMeta> m_systems[LifeCycleSize];

    template <typename... Ts>
    void process_one_many(Tag<One<Ts...>>, std::vector<QueryResultMeta>& results)
    {
        QueryResultMeta result;
        result.kind = QueryKind::One;

        (process_queries(Tag<Ts>{}, result), ...);

        results.push_back(result);
    }

    template <typename... Ts>
    void process_one_many(Tag<Many<Ts...>>, std::vector<QueryResultMeta>& results)
    {
        QueryResultMeta result;
        result.kind = QueryKind::Many;

        (process_queries(Tag<Ts>{}, result), ...);

        results.push_back(result);
    }

    template <typename T>
    void process_queries(Tag<T>, QueryResultMeta& result)
    {
        result.included_classes.push_back(T::get_static_hash_code());
    }

    template <typename... Ts>
    void process_queries(Tag<Child<Ts...>>, QueryResultMeta& result)
    {
        QueryResultMeta result2;
        result2.kind = QueryKind::Many; // TODO: Add Child/Children (or ChildOne/ChildMany) as equivalent One/Many
        (process_queries(Tag<Ts>{}, result2), ...);

        result.children.push_back(result2);
    }

    template <typename T>
    void process_queries(Tag<Not<T>>, QueryResultMeta& result)
    {
        result.excluded_classes.push_back(T::get_static_hash_code());
    }
};
