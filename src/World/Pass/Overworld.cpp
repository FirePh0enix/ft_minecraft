#include "World/Pass/Overworld.hpp"
#include "Engine.hpp"
#include "World/Registry.hpp"

// Biome OverworldBiomePass::generate_biome(int64_t x, int64_t y, int64_t z)
// {
//     (void)x;
//     (void)y;
//     (void)z;
//     return Biome::Plain;
// }

OverworldSurfacePass::OverworldSurfacePass()
{
}

BlockState OverworldSurfacePass::generate_block(int64_t x, int64_t y, int64_t z)
{
    static constexpr int64_t ocean_floor = 50;
    static constexpr int64_t sea_level = 70;

    // Continental noise give creates continent and oceans.
    float continental_noise = m_continental_noise.fractal<2>(glm::vec2(x, z) / 300.0f, 1.0, 1.0, 1.0, 1.0);
    float continental_height;
    if (continental_noise > 0)
    {
        continental_height = std::max(continental_noise * continental_noise * -33.0f + sea_level, float(ocean_floor));
    }
    else
    {
        continental_height = std::max(continental_noise * continental_noise * 7.0f + sea_level, float(ocean_floor));
    }

    // Mountain noises.
    float mountain_noise = m_mountain_noise.sample(glm::vec2(x, z) / 70.0f) * ((continental_noise + 1.0f) / 2.0f);
    float mountain_height = mountain_noise * 20.0f;

    int64_t height = int64_t(continental_height + mountain_height);

    // Fill the continent.
    if (y < height)
    {
        return BlockState(Blocks::dirt);
    }
    else if (y == height)
    {
        return BlockState(Blocks::dirt);
    }

    // Fill the ocean.
    if (y >= ocean_floor && y <= sea_level)
    {
        return BlockState(Blocks::water);
    }

    return BlockState();
}
