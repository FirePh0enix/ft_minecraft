#pragma once

#include "Core/Class.hpp"
#include "World/Block.hpp"
#include "World/Chunk.hpp"

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
