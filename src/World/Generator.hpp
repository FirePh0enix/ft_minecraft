#pragma once

#include "Block/Block.hpp"
#include "Core/Class.hpp"

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
    // virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) = 0;

    virtual BlockState generate_block(int64_t x, int64_t y, int64_t z) = 0;

    /**
     * Optional callback to initialize things which need a seed.
     */
    virtual void update_seed(uint64_t seed)
    {
        (void)seed;
    };
};

class SurfaceGenerationPass : public GenerationPass
{
    CLASS(SurfaceGenerationPass, GenerationPass);

public:
    // virtual BlockState generate_block(int64_t x, int64_t y, int64_t z);

    // virtual void generate(int64_t x, int64_t y, int64_t z, size_t block_index, Ref<Chunk>& chunk) override
    // {
    //     chunk->get_blocks()[block_index] = generate_block(x, y, z, chunk->get_biomes()[block_index]);
    // }
};
