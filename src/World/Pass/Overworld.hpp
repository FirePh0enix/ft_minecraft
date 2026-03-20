#pragma once

#include "World/Generator.hpp"

class OverworldBiomePass : public BiomeGenerationPass
{
    CLASS(OverworldBiomePass, BiomeGenerationPass);

public:
    virtual Biome generate_biome(int64_t x, int64_t y, int64_t z) override;
};

class OverworldSurfacePass : public SurfaceGenerationPass
{
    CLASS(OverworldSurfacePass, SurfaceGenerationPass);

public:
    virtual BlockState generate_block(int64_t x, int64_t y, int64_t z, Biome biome) override;
};
