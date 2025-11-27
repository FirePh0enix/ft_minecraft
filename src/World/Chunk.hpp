#pragma once

#include "Render/Driver.hpp"
#include "World/Block.hpp"

class World;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }
};

#define CHUNK_LOCAL_X(POS) (POS & 0xF)
#define CHUNK_LOCAL_Y(POS) ((POS >> 4) & 0xFF)
#define CHUNK_LOCAL_Z(POS) ((POS >> 12) & 0xF)

#define CHUNK_POS(X, Y, Z) ((X & 0xF) | ((Y & 0xFF) << 4) | ((Z & 0xFF) << 12))

struct ChunkBounds
{
    glm::ivec3 min;
    glm::ivec3 max;
};

struct ChunkGPUInfo
{
    int32_t chunk_x;
    int32_t chunk_z;
};
STRUCT(ChunkGPUInfo);

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * width * height;

    Chunk(int64_t x, int64_t z, const Ref<Shader>& surface_shader, const Ref<Shader>& visual_shader, World *world);

    inline BlockState get_block(size_t x, size_t y, size_t z) const
    {
        if (x > 15 || y > 255 || z > 15 || (z * width * height + y * width + x) >= m_blocks.size())
            return BlockState();
        return m_blocks[z * width * height + y * width + x];
    }

    inline void set_block(size_t x, size_t y, size_t z, BlockState state)
    {
        if (z * width * height + y * width + x > block_count)
            return;
        m_blocks[z * width * height + y * width + x] = state;
    }

    inline const std::vector<BlockState>& get_blocks() const
    {
        return m_blocks;
    }

    inline int64_t x() const
    {
        return m_x;
    }

    inline int64_t z() const
    {
        return m_z;
    }

    inline uint32_t get_block_count() const
    {
        return m_block_count;
    }

    inline ChunkBounds get_chunk_bounds() const
    {
        return ChunkBounds{.min = glm::ivec3(m_min_x, m_min_y, m_min_z), .max = glm::ivec3(m_max_x, m_max_y, m_max_z)};
    }

    Ref<Buffer> get_block_buffer() const { return m_gpu_blocks; }

    Ref<Material> get_visual_material() const { return m_visual_material; }

    /**
        Generate the chunk.
     */
    void generate();

private:
    std::vector<BlockState> m_blocks;
    int64_t m_x;
    int64_t m_z;

    Ref<ComputeMaterial> m_surface_material;
    Ref<Material> m_visual_material;

    Ref<Buffer> m_gpu_blocks;
    Ref<Buffer> m_cpu_blocks;

    // TODO: transparent blocks
    uint32_t m_block_count = 0;

    uint8_t m_min_x = 15;
    uint8_t m_min_y = 255;
    uint8_t m_min_z = 15;
    uint8_t m_max_x = 0;
    uint8_t m_max_y = 0;
    uint8_t m_max_z = 0;

    BlockState& get_block_ref(size_t x, size_t y, size_t z)
    {
        return m_blocks[z * width * height + y * width + x];
    }

    inline bool has_block(size_t x, size_t y, size_t z) const
    {
        if (x > 15 || y > 255 || z > 15)
            return false;
        return !m_blocks[z * width * height + y * width + x].is_air();
    }
};
