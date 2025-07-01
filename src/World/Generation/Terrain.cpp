#include "Terrain.hpp"
#include <print>

bool OverworldTerrainGenerator::has_block(int64_t x, int64_t y, int64_t z)
{
    float terrain_height = get_height(x, z);

    if ((float)y < terrain_height)
    {
        return true;
    }
    return false;
}

// Height depend on terrain type.
// Terrain type are not biome, they are somehow influence by continentalness, erosion and pv.
// TODO: Use minecraft wiki to identify the correct threshold.
float OverworldTerrainGenerator::get_height(int64_t x, int64_t z)
{
    float continentalness = get_continentalness_noise(x, z);
    float erosion = get_erosion_noise(x, z);
    float pv = get_peaks_and_valleys_noise(x, z);

    float ocean_height = generate_ocean_height(x, z);
    float beach_height = generate_beach_height(x, z);
    float mountain_height = generate_mountain_height(x, z);
    float plains_height = generate_plains_height(x, z);
    float hills_height = generate_hills_height(x, z);

    float height;

    if (continentalness < -0.6f)
    {
        height = ocean_height;
    }
    else if (continentalness < -0.4f)
    {
        // Calculate a blend factor to ensure that I'm not passing from a flat zone to mountain in one chunk.
        float blend = (continentalness + 0.6f) / 0.2f;

        // Smoothstep curves the transition, so it's not sharp.
        blend = glm::smoothstep(0.0f, 1.0f, blend);

        // Linearly interpolate between ocean and beach heights
        height = glm::mix(ocean_height, beach_height, blend);
    }
    else if (continentalness < -0.2f)
    {
        height = beach_height;
    }
    else
    {
        if (erosion < -0.4f)
        {
            height = mountain_height;
        }
        else if (erosion < 0.0f)
        {
            float blend = (erosion + 0.4f) / 0.4f;
            blend = glm::smoothstep(0.0f, 1.0f, blend);
            height = glm::mix(mountain_height, hills_height, blend);
        }
        else if (erosion < 0.2f)
        {
            height = hills_height;
        }
        else
        {
            float blend = (erosion - 0.2f) / 0.3f;
            blend = glm::clamp(blend, 0.0f, 1.0f);
            blend = glm::smoothstep(0.0f, 1.0f, blend);
            height = glm::mix(hills_height, plains_height, blend);
        }

        height += apply_pv_modification(pv, erosion);
    }

    return height;
}

// TODO: Find some docs about how I'm suppose to choose the effect of pv
float OverworldTerrainGenerator::apply_pv_modification(float pv, float erosion)
{
    float pv_intensity;

    if (erosion < -0.4f)
    {
        pv_intensity = 60.0f;
    }
    else if (erosion < 0.0f)
    {
        pv_intensity = 45.0f;
    }
    else if (erosion < 0.2f)
    {
        pv_intensity = 25.0f;
    }
    else
    {
        pv_intensity = 12.0f;
    }

    return pv * pv_intensity;
}

float OverworldTerrainGenerator::get_terrain_height(TerrainType biome, int64_t x, int64_t z)
{
    switch (biome)
    {
    case TerrainType::OCEAN:
        return generate_ocean_height(x, z);
    case TerrainType::BEACH:
        return generate_beach_height(x, z);
    case TerrainType::MOUNTAINS:
        return generate_mountain_height(x, z);
    case TerrainType::HILLS:
        return generate_hills_height(x, z);
    case TerrainType::PLAINS:
        return generate_plains_height(x, z);
    default:
        return 64.0f;
    }
}

float OverworldTerrainGenerator::get_continentalness_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(6, (float)x * 0.0001f, 0.0f, (float)z * 0.0001f);
}

float OverworldTerrainGenerator::get_erosion_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(4, (float)x * 0.0005f, 0.0f, (float)z * 0.0005f);
}

float OverworldTerrainGenerator::get_peaks_and_valleys_noise(int64_t x, int64_t z)
{
    float weirdness = m_noise.fractal(3, (float)x * 0.001f, 0.0f, (float)z * 0.001f);
    float abs_weirdness = std::abs(weirdness);
    float pv = 1.0f - std::abs(3.0f * abs_weirdness - 2.0f);
    return std::clamp(pv, -1.0f, 1.0f);
}

float OverworldTerrainGenerator::get_temperature_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(4, (float)x * 0.0002f, 0.0f, (float)z * 0.0002f);
}

float OverworldTerrainGenerator::get_humidity_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(4, (float)x * 0.0003f, 0.0f, (float)z * 0.0003f);
}

TerrainType OverworldTerrainGenerator::get_terrain_type(int64_t x, int64_t z)
{
    float continentalness = get_continentalness_noise(x, z);
    float erosion = get_erosion_noise(x, z);
    // Weirdness will be used to create some specials biome like peaks.
    float weirdness = get_peaks_and_valleys_noise(x, z);

    if (continentalness < -0.3f)
    {
        return TerrainType::OCEAN;
    }
    if (continentalness < 0.0f)
    {
        return TerrainType::BEACH;
    }
    if (erosion < -0.4f)
    {
        return TerrainType::MOUNTAINS;
    }
    else
    {
        return TerrainType::HILLS;
    }
}

float OverworldTerrainGenerator::generate_ocean_height(int64_t x, int64_t z)
{
    float ocean_noise = m_noise.fractal(2, (float)x * 0.003f, 0.0f, (float)z * 0.003f);
    float depth_variation = m_noise.fractal(3, (float)x * 0.001f, 0.0f, (float)z * 0.001f);

    return 64.0f - 25.0f + ocean_noise * 3.0f + depth_variation * 8.0f;
}

float OverworldTerrainGenerator::generate_beach_height(int64_t x, int64_t z)
{
    float beach_noise = m_noise.fractal(2, (float)x * 0.02f, 0.0f, (float)z * 0.02f);
    float coastal_slope = m_noise.fractal(1, (float)x * 0.0005f, 0.0f, (float)z * 0.0005f);

    return 64.0f - 3.0f + beach_noise * 2.0f + coastal_slope * 6.0f;
}

float OverworldTerrainGenerator::generate_mountain_height(int64_t x, int64_t z)
{
    float large_peaks = m_noise.fractal(3, (float)x * 0.002f, 0.0f, (float)z * 0.002f);
    float medium_detail = m_noise.fractal(4, (float)x * 0.008f, 0.0f, (float)z * 0.008f);
    float small_detail = m_noise.fractal(2, (float)x * 0.02f, 0.0f, (float)z * 0.02f);

    if (large_peaks > 0)
    {
        large_peaks = std::pow(large_peaks, 1.5f);
    }

    return 64.0f + 70.0f +
           large_peaks * 100.0f +
           medium_detail * 25.0f +
           small_detail * 8.0f;
}

float OverworldTerrainGenerator::generate_hills_height(int64_t x, int64_t z)
{
    float hill_base = m_noise.fractal(3, (float)x * 0.006f, 0.0f, (float)z * 0.006f);
    float hill_detail = m_noise.fractal(4, (float)x * 0.015f, 0.0f, (float)z * 0.015f);
    float small_bumps = m_noise.fractal(2, (float)x * 0.03f, 0.0f, (float)z * 0.03f);

    return 64.0f + 20.0f +
           hill_base * 35.0f +
           hill_detail * 12.0f +
           small_bumps * 4.0f;
}

float OverworldTerrainGenerator::generate_plains_height(int64_t x, int64_t z)
{
    float plains_base = m_noise.fractal(2, (float)x * 0.008f, 0.0f, (float)z * 0.008f);
    float gentle_rolls = m_noise.fractal(3, (float)x * 0.02f, 0.0f, (float)z * 0.02f);

    return 64.0f + 5.0f +
           plains_base * 12.0f +
           gentle_rolls * 5.0f;
}
