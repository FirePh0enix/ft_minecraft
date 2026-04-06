#pragma once

#include "Core/Class.hpp"
#include "World/Block.hpp"
#include "World/Chunk.hpp"

#include <mutex>
#include <semaphore>
#include <set>
#include <thread>
#include <vector>

class World;

class GenerationPass : public Object
{
    CLASS(GenerationPass, Object);

public:
    /**
     * @param x Global X position of the block
     * @param y Global Y position of the block
     * @param z Global Z position of the block
     * @param block_index Index of the block
     * @param chunk Chunk currently being generated
     */
    virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) = 0;

    /**
     * Optional callback to initialize things which need a seed.
     */
    virtual void update_seed(uint64_t seed)
    {
        (void)seed;
    };
};

class BiomeGenerationPass : public GenerationPass
{
    CLASS(BiomeGenerationPass, GenerationPass);

public:
    virtual Biome generate_biome(int64_t x, int64_t y, int64_t z) = 0;

    virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) override
    {
        chunk->get_biomes()[block_index] = generate_biome(x, y, z);
    }
};

class SurfaceGenerationPass : public GenerationPass
{
    CLASS(SurfaceGenerationPass, GenerationPass);

public:
    virtual BlockState generate_block(int64_t x, int64_t y, int64_t z, Biome biome) = 0;

    virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) override
    {
        chunk->get_blocks()[block_index] = generate_block(x, y, z, chunk->get_biomes()[block_index]);
    }
};

class FeaturesGenerationPass : public GenerationPass
{
    CLASS(FeaturesGenerationPass, GenerationPass);

public:
    virtual BlockState generate_features(int64_t x, int64_t y, int64_t z, BlockState state, Biome biome) = 0;

    virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) override
    {
        const BlockState state = chunk->get_blocks()[block_index];
        const Biome biome = chunk->get_biomes()[block_index];
        chunk->get_blocks()[block_index] = generate_features(x, y, z, state, biome);
    }
};

class Generator : public Object
{
    CLASS(Generator, Object);

public:
    Generator(World *world, size_t dimension);
    ~Generator();

    void add_pass(Ref<GenerationPass> pass);

    void set_distance(uint32_t distance);
    void load_around(int64_t x, int64_t z);

    Result<Ref<Chunk>> generate_chunk(int64_t x, int64_t z);

    void set_reference_pos(const glm::vec3& pos) { m_reference_position = pos; }

private:
    World *m_world;
    size_t m_dimension;

    std::vector<Ref<GenerationPass>> m_passes;

    std::thread m_load_thread;
    std::mutex m_load_orders_mutex;
    std::atomic_bool m_load_state = true;
    std::binary_semaphore m_load_orders_semaphore{0};
    std::set<ChunkPos> m_load_orders;

    ChunkPos pop_nearest_chunk(glm::vec3 position);
    ChunkPos pop_farsest_chunk(glm::vec3 position);

    std::thread m_unload_thread;
    std::mutex m_unload_orders_mutex;
    std::atomic_bool m_unload_orders_state = true;
    std::binary_semaphore m_unload_orders_semaphore{0};
    std::set<ChunkPos> m_unload_orders;

    glm::vec3 m_reference_position = glm::vec3();

    int64_t m_load_distance = 1;
    size_t m_max_chunk_count = 1;

    void request_multiple_load(View<ChunkPos> chunks);
    void request_unload(int64_t x, int64_t z);

    void load_thread();
    void unload_thread();
};
