#include "Terrain.hpp"
#include <print>

bool OverworldTerrainGenerator::has_block(int64_t x, int64_t y, int64_t z)
{
    float terrain_height = get_height(x, z);

    if (y < terrain_height)
    {
        return true;
    }
    return false;
}

float OverworldTerrainGenerator::get_height(int64_t x, int64_t z)
{
    SimplexNoise noise;
    const float base_height = 64.0f;
    const float height_variation = 20.0f;

    float noise1 = noise.fractal(4, x * 0.01f, 0.0f, z * 0.01f);

    float noise2 = noise.fractal(6, x * 0.05f, 0.0f, z * 0.05f) * 0.5f;

    float combined_noise = noise1 + noise2;

    float height = base_height + combined_noise * height_variation;

    return std::max(1.0f, height);
}