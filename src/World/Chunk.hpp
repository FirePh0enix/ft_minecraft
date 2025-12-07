#pragma once

#include "Render/Driver.hpp"
#include "Scene/Entity.hpp"
#include "World/Block.hpp"

class World;
class GridCollider;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    bool operator==(const ChunkPos& other) const
    {
        return std::tie(x, z) == std::tie(other.x, other.z);
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

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * width * height;

    Chunk(int64_t x, int64_t z);

    void set_blocks(const std::vector<BlockState>& blocks);
    void set_buffers(const Ref<Shader>& visual_shader, const Ref<Buffer>& position_buffer);

    void update_grid_collider(GridCollider *grid) const;

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

    Ref<Material> get_visual_material() const { return m_visual_material; }

    EntityId get_collision_entity_id() const { return m_collision_entity_id; }
    void set_collision_entity_id(EntityId id) { m_collision_entity_id = id; }

    float time_since_created = 0.0;

private:
    std::vector<BlockState> m_blocks;
    int64_t m_x;
    int64_t m_z;

    EntityId m_collision_entity_id;

    Ref<Buffer> m_block_buffer;
    Ref<Buffer> m_visibility_buffer;
    Ref<Material> m_visual_material;

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
