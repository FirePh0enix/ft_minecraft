#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Generator.hpp"

// class FlatBiomePass : public BiomeGenerationPass
// {
//     CLASS(FlatBiomePass, BiomeGenerationPass);

// public:
//     virtual Biome generate_biome(int64_t x, int64_t y, int64_t z) override;
// };

class FlatSurfacePass : public SurfaceGenerationPass
{
    CLASS(FlatSurfacePass, SurfaceGenerationPass);

public:
    FlatSurfacePass();

    virtual BlockState generate_block(int64_t x, int64_t y, int64_t z) override;

    virtual void update_seed(uint64_t seed) override { m_noise = SimplexNoise(seed); }

private:
    SimplexNoise m_noise = SimplexNoise(0);

    Id<Block> m_dirt_id;
};
