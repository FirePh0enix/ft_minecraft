#pragma once

#include "Core/Definitions.hpp"
#include "Core/Ref.hpp"
#include "Core/ThreadPool.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Ray.hpp"
#include "Render/Graph.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"

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
    int64_t x;
    int64_t y;
    int64_t z;
    Face face;
    glm::vec3 pos;
    float distance;
};

struct EntityRaycastResult
{
    Ref<Entity> entity;
    glm::vec3 hit_position;
    float distance;
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

    World(uint64_t seed, int type);
    ~World();

    void tick(float delta);

    /**
     *  Draw all the blocks and entities.
     */
    // void draw(RenderPassEncoder& encoder, bool include_entities = false);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z);

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

    std::optional<RaycastResult> raycast(const Ray& ray, float range);
    std::optional<EntityRaycastResult> raycast_entities(const Ray& ray, float range, Entity *self);

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
    uint32_t m_load_distance = 6;

    Ref<Camera> m_camera;

    void load_around_player();
};
