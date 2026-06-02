#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Generator.hpp"

// class OverworldBiomePass : public BiomeGenerationPass
// {
//     CLASS(OverworldBiomePass, BiomeGenerationPass);

// public:
//     virtual Biome generate_biome(int64_t x, int64_t y, int64_t z) override;
// };

class OverworldSurfacePass : public SurfaceGenerationPass
{
    CLASS(OverworldSurfacePass, SurfaceGenerationPass);

public:
    OverworldSurfacePass();

    virtual BlockState generate_block(int64_t x, int64_t y, int64_t z) override;

    virtual void update_seed(uint64_t seed) override
    {
        m_continental_noise = SimplexNoise(seed);
        m_mountain_noise = SimplexNoise(seed + 1);
    }

private:
    SimplexNoise m_continental_noise = SimplexNoise(0);
    SimplexNoise m_mountain_noise = SimplexNoise(0);
};
