#pragma once

#include "Render/Driver.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Visual.hpp"
#include "World/Dimension.hpp"

#include <mutex>

struct SimplexState
{
    std::array<uint8_t, 256> perms;
};
STRUCT(SimplexState);

class World : public VisualComponent
{
    CLASS(World, VisualComponent);

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;

    World(Ref<Mesh> mesh, Ref<Material> material);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z);

    void set_render_distance(uint32_t distance);

    virtual void encode_draw_calls(RenderPassEncoder& encoder, Camera& camera) override;

    inline int64_t get_distance() const
    {
        return m_distance;
    }

    inline std::mutex& get_chunk_add_mutex()
    {
        return m_chunks_add_mutex;
    }

    inline std::mutex& get_chunk_read_mutex()
    {
        return m_chunks_add_mutex;
    }

    inline std::mutex& get_buffer_mutex()
    {
        return m_buffers_mutex;
    }

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

    const Dimension& get_dimension(size_t index) const
    {
        return m_dims[index];
    }

    Dimension& get_dimension(size_t index)
    {
        return m_dims[index];
    }

    const Ref<Buffer>& get_perm_buffer() const
    {
        return m_permutation_buffer;
    }

    void load_chunk(int64_t x, int64_t z);
    void load_around(int64_t x, int64_t y, int64_t z);

private:
    std::array<Dimension, 2> m_dims;
    uint32_t m_distance = 0;

    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
    Ref<Shader> m_surface_shader;

    Ref<Buffer> m_position_buffer;
    Ref<Buffer> m_permutation_buffer;

    std::mutex m_chunks_add_mutex;
    std::mutex m_chunks_read_mutex;
    std::mutex m_buffers_mutex;

    // void update_visibility(int64_t x, int64_t y, int64_t z, bool recurse);
};
