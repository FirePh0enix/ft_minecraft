#include "World/Pass/Overworld.hpp"
#include "Engine.hpp"
#include "World/Registry.hpp"

Biome OverworldBiomePass::generate_biome(int64_t x, int64_t y, int64_t z)
{
    (void)x;
    (void)y;
    (void)z;
    return Biome::Plain;
}

OverworldSurfacePass::OverworldSurfacePass()
{
    m_dirt_id = Engine::get().block_registry().get_block_id("dirt");
    m_water_id = Engine::get().block_registry().get_block_id("water");
}

BlockState OverworldSurfacePass::generate_block(int64_t x, int64_t y, int64_t z, Biome biome)
{
    (void)biome;

    static constexpr int64_t ocean_floor = 50;
    static constexpr int64_t sea_level = 70;

    // Continental noise give creates continent and oceans.
    float continental_noise = m_noise.sample(glm::vec2(x, z) / 100.0f);
    float continental_height = std::max(continental_noise * 50.0f + sea_level, float(ocean_floor));

    int64_t height = int64_t(continental_height);

    // Fill the continent.
    if (y <= height)
    {
        return BlockState(m_dirt_id);
    }

    // Fill the ocean.
    if (y >= ocean_floor && y <= sea_level)
    {
        return BlockState(m_water_id);
    }

    return BlockState();
}
