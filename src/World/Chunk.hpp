#pragma once

#include "Core/Definitions.hpp"
#include "Render/Driver.hpp"
#include "World/Biome.hpp"
#include "World/Block.hpp"

#include "Render/WebGPU/DriverWebGPU.hpp"

#include <cstdint>

class World;

struct ChunkPos
{
    int64_t x;
    int64_t z;

    constexpr ChunkPos() : x(0), z(0) {}
    constexpr ChunkPos(int64_t x, int64_t z) : x(x), z(z) {}

    bool operator<(const ChunkPos& other) const
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    bool operator==(const ChunkPos& other) const
    {
        return std::tie(x, z) == std::tie(other.x, other.z);
    }
};

struct Model
{
    glm::mat4 model_matrix = glm::identity<glm::mat4>();
};

class Chunk : public Object
{
    CLASS(Chunk, Object);

public:
    struct Slice
    {
        Ref<Mesh> mesh = nullptr;
        Ref<Material> material = nullptr;
        Ref<Buffer> model_buffer = nullptr;
        Model model;
        bool empty = true;

        bool is_visible() const
        {
            // `empty` means the slice is only air blocks.
            // `mesh.is_null()` means the mesh was not created because no faces were visible.
            return !empty && !mesh.is_null();
        }
    };

    static constexpr int64_t width = 16;
    static constexpr int64_t width_with_overlap = width + 2;
    static constexpr int64_t height = 256;
    static constexpr int64_t block_count_with_overlap = width_with_overlap * height * width_with_overlap;
    static constexpr int64_t slice_count = height / width;

    Chunk(int64_t x, int64_t z, World *world);
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
    Result<void> build_simple_mesh(size_t slice);

    static ALWAYS_INLINE size_t linearize(int64_t x, int64_t y, int64_t z) { return (z + 1) * width_with_overlap * height + y * width_with_overlap + (x + 1); }
    static ALWAYS_INLINE size_t linearize_with_overlap(int64_t x, int64_t y, int64_t z) { return z * width_with_overlap * height + y * width_with_overlap + x; }

private:
    BlockState *m_blocks;
    Biome *m_biomes;
    Slice *m_slices;

    int64_t m_x;
    int64_t m_z;
};
