#include "Terrain.hpp"
#include "Core/Print.hpp"
#include <cstdint>
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

    TerrainShape shape = get_terrain_shape(x, z);

    if (shape == TerrainShape::OCEAN)
    {
        // Calculate a blend factor to ensure that I'm not passing from a flat zone to mountain in one chunk.
        float blend = (continentalness + 0.6f) / 0.2f;

        // Smoothstep curves the transition, so it's not sharp.
        blend = glm::smoothstep(0.0f, 1.0f, blend);

        // Linearly interpolate between ocean and beach heights
        height = glm::mix(ocean_height, beach_height, blend);
    }
    else if (shape == TerrainShape::BEACH)
    {
        height = beach_height;
    }

    else if (shape == TerrainShape::MOUNTAINS)
    {
        float blend = (erosion + 0.4f) / 0.4f;
        blend = glm::smoothstep(0.0f, 1.0f, blend);
        height = glm::mix(mountain_height, hills_height, blend);
    }
    else if (shape == TerrainShape::HILLS)
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

TerrainShape OverworldTerrainGenerator::get_terrain_shape(int64_t x, int64_t z)
{
    float continentalness = get_continentalness_noise(x, z);
    float erosion = get_erosion_noise(x, z);
    // Weirdness will be used to create some special terrain type like peaks.
    float weirdness = get_peaks_and_valleys_noise(x, z);

    if (continentalness < -0.3f)
    {
        return TerrainShape::OCEAN;
    }
    if (continentalness < 0.0f)
    {
        return TerrainShape::BEACH;
    }
    if (erosion < -0.4f)
    {
        return TerrainShape::MOUNTAINS;
    }
    else
    {
        return TerrainShape::HILLS;
    }
}

float OverworldTerrainGenerator::generate_ocean_height(int64_t x, int64_t z)
{
    float ocean_noise = m_noise.fractal(2, (float)x * 0.003f, 0.0f, (float)z * 0.003f);
    float depth_variation = m_noise.fractal(3, (float)x * 0.001f, 0.0f, (float)z * 0.001f);

    return base_height - 25.0f + ocean_noise * 3.0f + depth_variation * 8.0f;
}

float OverworldTerrainGenerator::generate_beach_height(int64_t x, int64_t z)
{
    float beach_noise = m_noise.fractal(2, (float)x * 0.02f, 0.0f, (float)z * 0.02f);
    float coastal_slope = m_noise.fractal(1, (float)x * 0.0005f, 0.0f, (float)z * 0.0005f);

    return base_height - 3.0f + beach_noise * 2.0f + coastal_slope * 6.0f;
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

    return base_height + 70.0f + large_peaks * 100.0f + medium_detail * 25.0f + small_detail * 8.0f;
}

float OverworldTerrainGenerator::generate_hills_height(int64_t x, int64_t z)
{
    float hill_base = m_noise.fractal(3, (float)x * 0.006f, 0.0f, (float)z * 0.006f);
    float hill_detail = m_noise.fractal(4, (float)x * 0.015f, 0.0f, (float)z * 0.015f);
    float small_bumps = m_noise.fractal(2, (float)x * 0.03f, 0.0f, (float)z * 0.03f);

    return base_height + 20.0f + hill_base * 35.0f + hill_detail * 12.0f + small_bumps * 4.0f;
}

float OverworldTerrainGenerator::generate_plains_height(int64_t x, int64_t z)
{
    float plains_base = m_noise.fractal(2, (float)x * 0.008f, 0.0f, (float)z * 0.008f);
    float gentle_rolls = m_noise.fractal(3, (float)x * 0.02f, 0.0f, (float)z * 0.02f);

    return base_height + 5.0f + plains_base * 12.0f + gentle_rolls * 5.0f;
}

Biome OverworldTerrainGenerator::get_biome(BiomeNoise& biome_noise)
{
    Biome biome{};
    auto continentalness_level = get_continentalness_level(biome_noise.continentalness);

    if (continentalness_level == ContinentalnessLevel::Ocean || continentalness_level == ContinentalnessLevel::DeepOcean)
    {
        biome.non_inland_biome = get_non_inland_biome(biome_noise);
    }
    else
    {
        biome.inland_biome = get_inland_biome(biome_noise);
    }

    return biome;
}

// If its a ocean shape.
// TODO: Add the 2 others oceans.
NonInlandBiome OverworldTerrainGenerator::get_non_inland_biome(BiomeNoise& biome_noise)
{
    return NonInlandBiome::Ocean;
}

InlandBiome OverworldTerrainGenerator::get_inland_biome(BiomeNoise& biome_noise)
{
    auto continentalness_level = get_continentalness_level(biome_noise.continentalness);
    auto peaks_and_valleys_level = get_peaks_and_valleys_level(biome_noise.peaks_and_valleys);
    uint32_t temperature_level = get_temperature_level(biome_noise.temperature_noise);
    uint32_t erosion_level = get_erosion_level(biome_noise.erosion);
    uint32_t humidity_level = get_humidity_level(biome_noise.humidity_noise);

    if (continentalness_level == ContinentalnessLevel::Coast)
    {
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Valleys)
        {
            if (temperature_level == 0)
            {
                return InlandBiome::FrozenRiver;
            }
            return InlandBiome::River;
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid)
        {
            if (erosion_level >= 0 && erosion_level <= 2)
            {
                return InlandBiome::StonyShore;
            }
            if (erosion_level >= 3 && erosion_level <= 4)
            {
                return get_beach_biome(temperature_level);
            }
            if (erosion_level == 5)
            {
                if (biome_noise.peaks_and_valleys < 0)
                {
                    return get_beach_biome(temperature_level);
                }
                return get_middle_biome(temperature_level, humidity_level);
            }
            return get_beach_biome(temperature_level);
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return InlandBiome::JaggedPeaks;
                    }
                    return InlandBiome::FrozenPeaks;
                }
                return InlandBiome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }
    else if (continentalness_level == ContinentalnessLevel::NearInland)
    {
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Valleys)
        {
            if (temperature_level == 0)
            {
                return InlandBiome::FrozenRiver;
            }
            return InlandBiome::River;
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid || peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return InlandBiome::JaggedPeaks;
                    }
                    return InlandBiome::FrozenPeaks;
                }
                return InlandBiome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }
    else if (continentalness_level == ContinentalnessLevel::MidInland || continentalness_level == ContinentalnessLevel::FarInland)
    {
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Valleys)
        {
            if (erosion_level >= 1 && erosion_level <= 1)
            {
                return get_middle_biome(temperature_level, humidity_level);
            }
            else
            {
                if (temperature_level == 0)
                {
                    return InlandBiome::FrozenRiver;
                }
                return InlandBiome::River;
            }
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return InlandBiome::JaggedPeaks;
                    }
                    return InlandBiome::FrozenPeaks;
                }
                return InlandBiome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
        else if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level >= 0 && erosion_level <= 1)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return InlandBiome::JaggedPeaks;
                    }
                    return InlandBiome::FrozenPeaks;
                }
                return InlandBiome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }

    return get_middle_biome(temperature_level, humidity_level);
}

InlandBiome OverworldTerrainGenerator::get_beach_biome(uint32_t temperature)
{
    if (temperature == 4)
    {
        return InlandBiome::Desert;
    }
    return InlandBiome::Beach;
}

InlandBiome OverworldTerrainGenerator::get_middle_biome(uint32_t temperature, uint32_t humidity)
{
    if (temperature == 0)
    {
        if (humidity == 4)
        {
            return InlandBiome::Taiga;
        }
        return InlandBiome::SnowyPlains;
    }
    if (temperature >= 1 && temperature <= 2)
    {
        if (humidity >= 0 && humidity <= 2)
        {
            return InlandBiome::Plains;
        }
        return InlandBiome::Taiga;
    }
    if (temperature == 3)
    {
        if (humidity >= 0 && humidity <= 1)
        {
            return InlandBiome::Savanna;
        }
        return InlandBiome::Plains;
    }
    return InlandBiome::Desert;
}

ContinentalnessLevel OverworldTerrainGenerator::get_continentalness_level(float continentalness)
{
    if (continentalness >= -1.0f && continentalness < 0.455f)
    {
        return ContinentalnessLevel::DeepOcean;
    }
    else if (continentalness >= -0.455f && continentalness < -0.19f)
    {
        return ContinentalnessLevel::Ocean;
    }
    else if (continentalness >= -0.19f && continentalness < -0.11f)
    {
        return ContinentalnessLevel::Coast;
    }
    else if (continentalness >= -0.11f && continentalness < 0.3f)
    {
        return ContinentalnessLevel::NearInland;
    }
    else if (continentalness >= 0.03f && continentalness < 0.3f)
    {
        return ContinentalnessLevel::MidInland;
    }

    return ContinentalnessLevel::FarInland;
}

uint32_t OverworldTerrainGenerator::get_erosion_level(float erosion)
{
    if (erosion >= -1.0f && erosion < -0.78f)
    {
        return 0;
    }
    else if (erosion >= -0.78f && erosion < -0.375f)
    {
        return 1;
    }
    else if (erosion >= -0.375f && erosion < -0.2225f)
    {
        return 2;
    }
    else if (erosion >= -0.2225f && erosion < 0.05f)
    {
        return 3;
    }
    else if (erosion >= 0.05f && erosion < 0.45f)
    {
        return 4;
    }
    else if (erosion >= 0.45f && erosion < 0.55f)
    {
        return 5;
    }
    return 6;
}
PeaksAndValleysLevel OverworldTerrainGenerator::get_peaks_and_valleys_level(float peaks_and_valleys)
{
    if (peaks_and_valleys >= -1.0f && peaks_and_valleys < -0.85f)
    {
        return PeaksAndValleysLevel::Valleys;
    }
    else if (peaks_and_valleys >= -0.85f && peaks_and_valleys < -0.2f)
    {
        return PeaksAndValleysLevel::Low;
    }
    else if (peaks_and_valleys >= -0.2f && peaks_and_valleys < 0.2f)
    {
        return PeaksAndValleysLevel::Mid;
    }
    else if (peaks_and_valleys >= 0.2f && peaks_and_valleys < 0.7f)
    {
        return PeaksAndValleysLevel::High;
    }
    return PeaksAndValleysLevel::Peaks;
}

uint32_t OverworldTerrainGenerator::get_temperature_level(float temperature)
{
    if (temperature >= -1.0f && temperature < -0.45f)
    {
        return 0;
    }
    else if (temperature >= -0.45f && temperature < -0.15f)
    {
        return 1;
    }
    else if (temperature >= -0.15f && temperature < 0.2f)
    {
        return 2;
    }
    else if (temperature >= 0.2f && temperature < 0.55f)
    {
        return 3;
    }
    return 4;
}

uint32_t OverworldTerrainGenerator::get_humidity_level(float humidity)
{
    if (humidity >= -1.0f && humidity < -0.35f)
    {
        return 0;
    }
    else if (humidity >= -0.35f && humidity < -0.1f)
    {
        return 1;
    }
    else if (humidity >= -0.1f && humidity < 0.1f)
    {
        return 2;
    }
    else if (humidity >= 0.1f && humidity < 0.3f)
    {
        return 3;
    }
    return 4;
}