#pragma once

#include "Block/Block.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/Definitions.hpp"
#include "Core/String.hpp"
#include "Render/Renderer.hpp"
#include "Variant.hpp"
#include "World/Biome.hpp"

#include <cstdint>

class World;
class Dimension;

class ChunkPos
{
public:
    int64_t x;
    int64_t z;

    constexpr ChunkPos() : x(0), z(0) {}
    constexpr ChunkPos(int64_t x, int64_t z) : x(x), z(z) {}

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    bool operator>(const ChunkPos& other) const
    {
        return std::tie(x, z) > std::tie(other.x, other.z);
    }

    bool operator==(const ChunkPos& other) const
    {
        return std::tie(x, z) == std::tie(other.x, other.z);
    }
};

struct BlockTags
{
    HashMap<String, Variant> tags;
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    friend class World;
    friend class Dimension;

    struct Slice
    {
        Ref<Mesh> mesh = nullptr;
        Ref<Mesh> water_mesh = nullptr;

        Ref<BindGroup> mesh_bg;
        Ref<BindGroup> mesh_shadowmap_bg;
        Ref<BindGroup> water_bg;
    };

    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * height * width;
    static constexpr int64_t slice_count = height / width;

    Chunk(Dimension *dim, int64_t x, int64_t z);
    Chunk(const Chunk&) = delete;
    ~Chunk();

    ALWAYS_INLINE BlockState get_block(int64_t x, int64_t y, int64_t z) const { return m_blocks[linearize(x, y, z)]; }
    void set_block(int64_t x, int64_t y, int64_t z, BlockState state);

    ALWAYS_INLINE const BlockState *get_blocks() const { return m_blocks; }
    ALWAYS_INLINE BlockState *get_blocks() { return m_blocks; }

    ALWAYS_INLINE const Biome *get_biomes() const { return m_biomes; }
    ALWAYS_INLINE Biome *get_biomes() { return m_biomes; }

    ALWAYS_INLINE int64_t x() const { return m_x; }
    ALWAYS_INLINE int64_t z() const { return m_z; }

    ALWAYS_INLINE ChunkPos pos() const { return ChunkPos(m_x, m_z); }

    const Slice *get_slices() const { return m_slices; }
    Slice *get_slices() { return m_slices; }

    ALWAYS_INLINE Ref<Buffer> get_instance_buffer() const { return m_uniform_buffer; }

    Result<void> build_simple_mesh(size_t slice, const Map<ChunkPos, Ref<Chunk>>& chunks);
    Result<void> build_water_mesh(size_t slice, const Map<ChunkPos, Ref<Chunk>>& chunks);

    bool is_modified() const { return m_modified; }
    void clear_modified() { m_modified = false; }

    void set_tag(glm::i64vec3 pos, const StringView& name, Variant v, bool rebuild = true);
    void remove_tag(glm::i64vec3 pos, const StringView& name, bool rebuild = true);
    Option<Variant> get_tag(glm::i64vec3 pos, const StringView& name) const;
    Option<Variant> get_tag(uint16_t index, const StringView& name) const;

    void merge_tag(uint16_t index, const BlockTags& tags);

    static ALWAYS_INLINE size_t linearize(int64_t x, int64_t y, int64_t z) { return z * width * height + y * width + x; }

private:
    BlockState *m_blocks;
    Biome *m_biomes;
    Slice *m_slices;

    Dimension *m_dim;

    Map<int64_t, BlockTags> m_tags;

    Ref<Buffer> m_uniform_buffer;

    int64_t m_x;
    int64_t m_z;

    bool m_modified : 1 = false;
};
