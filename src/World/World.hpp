#pragma once

#include "Core/Definitions.hpp"
#include "Entity/Camera.hpp"
#include "Ray.hpp"
#include "Render/Driver.hpp"
#include "Render/Graph.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"
#include "World/Generator.hpp"

struct Environment
{
    glm::mat4 view_matrix = glm::identity<glm::mat4>();
    glm::f32 time = 0.0;
};

struct RaycastResult
{
    int64_t x;
    int64_t y;
    int64_t z;
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
    ~World();

    void tick(float delta);

    /**
     *  Draw all the blocks and entities.
     */
    void draw(RenderPassEncoder& encoder, bool include_entities = false);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t y, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t y, int64_t z);

    void remove_chunk(int64_t x, int64_t y, int64_t z)
    {
        m_dims[0].remove_chunk(x, y, z);
    }

    void add_chunk(int64_t x, int64_t y, int64_t z, const Ref<Chunk>& chunk)
    {
        m_dims[0].add_chunk(x, y, z, chunk);
    }

    bool is_chunk_loaded(int64_t x, int64_t y, int64_t z) const
    {
        return m_dims[0].has_chunk(x, y, z);
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
        m_dims[dimension].add_entity(entity);
    }

    ALWAYS_INLINE const Ref<Buffer>& get_env_buffer() const { return m_env_buffer; };

    std::optional<RaycastResult> raycast(const Ray& ray, float range);

    static EntityId next_id()
    {
        static uint32_t id = 0;
        id++;
        return EntityId(id);
    }

private:
    uint64_t m_seed;

    std::array<Dimension, max_dimensions> m_dims;
    std::array<Ref<Generator>, max_dimensions> m_generators;

    Ref<Camera> m_camera;
    Ref<Shader> m_shader;

    Ref<Buffer> m_env_buffer;
    Environment m_env;

    void update_environment_buffer();
};
