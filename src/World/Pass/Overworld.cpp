#include "World/Pass/Overworld.hpp"

Biome OverworldBiomePass::generate_biome(int64_t x, int64_t y, int64_t z)
{
    (void)x;
    (void)y;
    (void)z;
    return Biome::Plain;
}

BlockState OverworldSurfacePass::generate_block(int64_t x, int64_t y, int64_t z, Biome biome)
{
    (void)x;
    (void)z;
    (void)biome;

    if (y <= 1)
        return BlockState(2);
    return BlockState();
}
