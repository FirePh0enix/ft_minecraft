#pragma once

#include "Render/Driver.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Visual.hpp"
#include "World/Dimension.hpp"

#include <mutex>

struct BlockInstanceData
{
    glm::vec3 position;
    glm::uvec3 textures;
    uint8_t visibility;
    uint8_t gradient = 0;
    uint8_t gradient_type = 0;
    uint8_t pad = 0;
};

class World : public VisualComponent
{
    CLASS(World, VisualComponent);

    struct BufferInfo
    {
        Ref<Buffer> buffer;
        bool used = false;
    };

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;

    World(Ref<Mesh> mesh, Ref<Material> material);

    BlockState get_block_state(int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int64_t x, int64_t y, int64_t z, BlockState state);

    std::optional<const Chunk *> get_chunk(int64_t x, int64_t z) const;
    std::optional<Chunk *> get_chunk(int64_t x, int64_t z);

    void set_render_distance(uint32_t distance);

    inline Ref<Buffer> get_buffer(size_t index) const
    {
        return m_buffers[index].buffer;
    }

    /**
     * @brief Returns the index of the first free chunk buffer if any.
     *
     * This function is thread-safe.
     */
    std::optional<size_t> acquire_buffer();

    /**
     * @brief Mark a buffer as unused.
     */
    void free_buffer(size_t index);

    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) override;

    inline int64_t get_distance() const
    {
        return m_distance;
    }

    inline std::vector<Chunk>& get_chunks()
    {
        return m_dims[0].get_chunks();
    }

    inline std::mutex& get_chunk_mutex()
    {
        return m_chunks_mutex;
    }

    inline std::mutex& get_buffer_mutex()
    {
        return m_buffers_mutex;
    }

    inline const std::vector<Chunk>& get_chunks() const
    {
        return m_dims[0].get_chunks();
    }

    void remove_chunk(int64_t x, int64_t z)
    {
        auto iter = std::find_if(m_dims[0].get_chunks().begin(), m_dims[0].get_chunks().end(), [x, z](const Chunk& c)
                                 { return c.x() == x && c.z() == z; });
        if (iter != m_dims[0].get_chunks().end())
            m_dims[0].get_chunks().erase(iter);
    }

    bool is_chunk_loaded(int64_t x, int64_t z) const
    {
        for (const auto& chunk : get_chunks())
        {
            if (chunk.x() == x && chunk.z() == z)
                return true;
        }
        return false;
    }

private:
    std::array<Dimension, 2> m_dims;
    std::vector<BufferInfo> m_buffers;
    uint32_t m_distance = 0;

    Ref<Mesh> m_mesh;
    Ref<Material> m_material;

    std::mutex m_chunks_mutex;
    std::mutex m_buffers_mutex;

    void update_visibility(int64_t x, int64_t y, int64_t z, bool recurse);
};
