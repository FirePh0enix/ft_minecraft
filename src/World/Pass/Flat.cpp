#include "World/Pass/Flat.hpp"
#include "World/Registry.hpp"

Biome FlatBiomePass::generate_biome(int64_t x, int64_t y, int64_t z)
{
    (void)x;
    (void)y;
    (void)z;
    return Biome::Plain;
}

FlatSurfacePass::FlatSurfacePass()
{
    m_dirt_id = BlockRegistry::get_block_id("dirt");
}

BlockState FlatSurfacePass::generate_block(int64_t x, int64_t y, int64_t z, Biome biome)
{
    (void)biome;
    (void)x;
    (void)z;

    if (y <= 10)
        return BlockState(m_dirt_id);
    return BlockState();
}
