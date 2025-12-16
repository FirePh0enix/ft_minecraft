#pragma once

#include "Render/Driver.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Visual.hpp"
#include "World/Dimension.hpp"

#include <mutex>

class RigidBody;

class World : public VisualComponent
{
    CLASS(World, VisualComponent);

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;

    World(Ref<Mesh> mesh, Ref<Shader> visual_shader, uint64_t seed);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z);

    virtual void encode_draw_calls(RenderPassEncoder& encoder, Camera& camera) override;

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
    std::mutex& get_chunk_mutex() { return m_chunk_mutex; }

    static void sync_physics_world(const Query<Many<Transformed3D, RigidBody>, One<World>>& query, Action& action);

private:
    uint64_t m_seed;
    std::array<Dimension, 2> m_dims;

    Ref<Mesh> m_mesh;
    Ref<Shader> m_visual_shader;
    Ref<Shader> m_visibility_shader;
    Ref<Shader> m_surface_shader;

    Ref<Buffer> m_position_buffer;

    // Mutex to guard adding/removing/drawing chunks.
    std::mutex m_chunk_mutex;
};
