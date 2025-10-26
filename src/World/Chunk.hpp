#pragma once

#include "Render/Driver.hpp"
#include "World/Biome.hpp"
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

struct BlockInstanceData
{
    glm::vec3 position;
    glm::uvec3 textures;
    uint8_t visibility;
    Biome biome : 8;
    GradientType gradient_type;
    uint8_t pad;
} __attribute__((aligned(16)));

struct ChunkGPUInfo
{
    uint32_t chunk_x;
    uint32_t chunk_z;
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * width * height;

    Chunk(int64_t x, int64_t z, Ref<Shader> shader)
        : m_x(x), m_z(z)
    {
        m_blocks.resize(block_count);
        m_surface_material = ComputeMaterial::create(shader);
        m_instance_buffer = RenderingDriver::get()->create_buffer(block_count * sizeof(BlockInstanceData), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex | BufferUsageFlagBits::Storage).value();

        m_gpu_blocks = RenderingDriver::get()->create_buffer(block_count * sizeof(BlockState), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::Storage).value();
        m_gpu_info = RenderingDriver::get()->create_buffer(sizeof(ChunkGPUInfo), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Uniform).value();

        ChunkGPUInfo info(x, z);
        RenderingDriver::get()->update_buffer(m_gpu_info, View(info).as_bytes());

        m_surface_material->set_param("info", m_gpu_info);
        m_surface_material->set_param("blocks", m_gpu_blocks);
        m_surface_material->set_param("instances", m_instance_buffer);
    }

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

    void set_biome(int64_t x, int64_t z, Biome biome)
    {
        m_biomes[x + z * width] = biome;
    }

    const Ref<Buffer>& get_instance_buffer() const
    {
        return m_instance_buffer;
    }

    /**
        Generate the chunk.
     */
    void generate();

    void compute_full_visibility(World *world);
    void compute_visibility(const World *world, int64_t x, int64_t y, int64_t z);

    void compute_axis_neighbour_visibility(const Ref<World>& world, const Ref<Chunk>& neighbour);

    void update_instance_buffer();

private:
    std::vector<BlockState> m_blocks;
    std::array<Biome, 16 * 16> m_biomes = {Biome::Plains};
    int64_t m_x;
    int64_t m_z;

    Ref<ComputeMaterial> m_surface_material;
    Ref<Buffer> m_instance_buffer;

    Ref<Buffer> m_gpu_blocks;
    Ref<Buffer> m_gpu_info;

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
