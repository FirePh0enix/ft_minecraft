#pragma once

#include "Block/Block.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Variant.hpp"
#include "World/Biome.hpp"
#include "stdext.hpp"

#include <cstdint>
#include <set>

class World;
class Dimension;

class Mesh;
class BindGroup;
class Buffer;
class Texture;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    constexpr ChunkPos() : x(0), z(0) {}
    constexpr ChunkPos(int64_t x, int64_t z) : x(x), z(z) {}

    bool operator<(ChunkPos other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }
};

struct BlockPos
{
    int64_t x;
    int64_t y;
    int64_t z;

    constexpr BlockPos() : x(0), y(0), z(0) {}
    constexpr BlockPos(int64_t x, int64_t y, int64_t z) : x(x), y(y), z(z) {}

    bool operator<(BlockPos other) const
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    operator glm::i64vec3() const
    {
        return {x, y, z};
    }
};

class Chunk
{
public:
    friend class World;
    friend class Dimension;

    struct Slice
    {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Mesh> water_mesh;

        std::shared_ptr<BindGroup> mesh_bg;
        std::shared_ptr<BindGroup> mesh_shadowmap_bg;
        std::shared_ptr<BindGroup> water_bg;
    };

    static constexpr int64_t width = 16;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count = width * height * width;
    static constexpr int64_t slice_count = height / width;

    Chunk() {}
    Chunk(Dimension *dim, int64_t x, int64_t z);
    Chunk(const Chunk&) = delete;
    ~Chunk();

    void update_instance_buffer(glm::dvec3 position, uint32_t slice_index);

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

    ALWAYS_INLINE std::shared_ptr<Buffer> get_instance_buffer() const { return m_uniform_buffer; }

    Result<std::shared_ptr<Mesh>> build_simple_mesh(size_t slice, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks);
    Result<std::shared_ptr<Mesh>> build_water_mesh(size_t slice, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks);

    bool is_modified() const { return m_modified; }
    void clear_modified() { m_modified = false; }

    void set_tag(glm::i64vec3 pos, std::string_view name, Variant v);
    void remove_tag(glm::i64vec3 pos, std::string_view name);
    std::optional<Variant> get_tag(glm::i64vec3 pos, std::string_view name) const;
    std::optional<Variant> get_tag(uint16_t index, std::string_view name) const;
    void merge_tag(uint16_t index, const stdext::string_map<Variant>& tags);

    const std::set<BlockPos>& get_non_coventional_blocks() const { return m_non_conventional_blocks; }

    static ALWAYS_INLINE size_t linearize(int64_t x, int64_t y, int64_t z) { return z * width * height + y * width + x; }

private:
    BlockState *m_blocks = nullptr;
    Biome *m_biomes = nullptr;
    Slice *m_slices = nullptr;

    Dimension *m_dim = nullptr;

    std::map<int64_t, stdext::string_map<Variant>> m_tags;
    std::set<BlockPos> m_non_conventional_blocks;

    std::shared_ptr<Buffer> m_uniform_buffer;

    int64_t m_x = 0;
    int64_t m_z = 0;

    bool m_modified : 1 = false;
};
