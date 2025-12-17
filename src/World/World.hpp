#pragma once

#include "Entity/Camera.hpp"
#include "Render/Driver.hpp"
#include "Render/Graph.hpp"
#include "World/Dimension.hpp"
#include "World/Generator.hpp"

class World : public Object
{
    CLASS(World, Object);

    friend class Generator;

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;

    World(Ref<Mesh> mesh, Ref<Shader> visual_shader, uint64_t seed);

    void tick(float delta);

    /**
     *  Draw all the blocks and entities.
     */
    void draw(RenderPassEncoder& encoder);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z);

    void remove_chunk(int64_t x, int64_t z)
    {
        m_dims[0].remove_chunk(x, z);
    }

    void add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
    {
        m_dims[0].add_chunk(x, z, chunk);
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

    const Ref<Buffer>& get_position_buffer() const { return m_position_buffer; }

    void set_active_camera(Ref<Camera> camera) { m_camera = camera; }

    static EntityId next_id()
    {
        static uint32_t id = 0;
        id++;
        return EntityId(id);
    }

private:
    uint64_t m_seed;

    std::array<Dimension, 2> m_dims;
    std::array<Ref<Generator>, 2> m_generators;

    Ref<Camera> m_camera;

    Ref<Mesh> m_mesh;
    Ref<Shader> m_visual_shader;
    Ref<Shader> m_visibility_shader;

    Ref<Buffer> m_position_buffer;
};
