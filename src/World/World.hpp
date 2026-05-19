#pragma once

#include "Core/Definitions.hpp"
#include "Core/Ref.hpp"
#include "Core/ThreadPool.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Ray.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"

#include <cstddef>

enum WorldPresetType
{
    WorldPresetFlat,
    WorldPresetNormal,
};

enum class Face
{
    PosX,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ
};

inline glm::vec3 face_normal(Face face)
{
    const glm::vec3 normals[6]{
        glm::vec3(1, 0, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
    };
    return normals[(size_t)face];
}

struct RaycastResult
{
    glm::vec3 pos;
    float distance;
    /**
     * If true, `entity` is a valid reference to an entity else `block_pos` is valid.
     */
    bool hit_entity;

    glm::i64vec3 block_pos;
    Ref<Entity> entity;
};

class World : public Object
{
    CLASS(World, Object);

    friend class Generator;
    friend class Chunk;

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;
    static constexpr size_t max_dimensions = 2;

    World(uint64_t seed);
    World(uint64_t seed, int type);
    ~World();

    static Result<Ref<World>> create_proxy(uint64_t seed);

    void tick(float delta);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z);

    void remove_chunk(int64_t x, int64_t z)
    {
        m_dims[0].remove_chunk(x, z);
    }

    void remove_entity(size_t dim, EntityId id)
    {
        m_dims[dim].remove_entity(id);
    }

    void add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
    {
        EXPECT(m_dims[0].add_chunk(x, z, chunk));
    }

    bool is_chunk_loaded(int64_t x, int64_t z) const
    {
        return m_dims[0].has_chunk(x, z);
    }

    uint64_t seed() const { return m_seed; }

    const Dimension& get_dimension(size_t index) const
    {
        return m_dims[index];
    }

    Dimension& get_dimension(size_t index)
    {
        return m_dims[index];
    }

    void set_active_camera(Ref<Camera> camera);
    ALWAYS_INLINE Ref<Camera> get_active_camera() { return m_camera; }

    void add_entity(size_t dimension, Ref<Entity> entity)
    {
        entity->m_world = this;
        entity->m_dimension = dimension;
        entity->on_ready();
        EXPECT(m_dims[dimension].add_entity(entity));
    }

    Ref<Entity> get_entity(EntityId id) const
    {
        return m_dims[0].get_entity(id);
    }

    glm::vec3 get_spawn_position() const { return m_spawn_position; }

    /**
     * Cast a ray through the world and returns the first thing it hit.
     *
     * @param ray Describe the ray direction and origin
     * @param range Size of the ray
     * @return true if the ray hit something
     */
    bool raycast(const Ray& ray, float range, RaycastResult& result);

    /**
     * Break the block and drop an item corresponding to it.
     */
    void break_block(int64_t x, int64_t y, int64_t z);

    static EntityId next_id()
    {
        static uint32_t id = 0;
        id++;
        return EntityId(id);
    }

private:
    uint64_t m_seed;

    std::array<Dimension, max_dimensions> m_dims;

    // Thread pool used for chunk loading/unloading.
    ThreadPool m_generation_thread_pool;
    int32_t m_load_distance = 8;

    Ref<Camera> m_camera;
    bool m_proxy = false;

    glm::vec3 m_spawn_position = glm::vec3();

    void find_safe_spawn();
    void load_around_player();

    void load_one_chunk(ChunkPos pos);
    void unload_one_chunk(ChunkPos pos);
};
