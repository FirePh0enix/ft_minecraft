#include "Terrain.hpp"
#include "Core/Print.hpp"
#include "glm/common.hpp"
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

    TerrainShape shape = get_terrain_shape(x, z);

    if (shape == TerrainShape::OCEAN || shape == TerrainShape::RIVER)
    {
        return sea_level;
    }
    if (shape == TerrainShape::BEACH)
    {
        float ocean_height = sea_level;
        float beach_height = generate_beach_height(x, z);
        // + 1 because negative range.
        float blend = (continentalness + 1.0f) / (0.455f - 0.19f);

        return blend_heights(ocean_height, beach_height, blend);
    }
    if (shape == TerrainShape::PLAINS)
    {
        float plains_height = generate_plains_height(x, z);
        float hills_height = generate_hills_height(x, z);

        float blend = (erosion - 0.05f) / (0.45f - 0.05f);

        return blend_heights(hills_height, plains_height, blend);
    }
    if (shape == TerrainShape::MOUNTAINS)
    {
        float mountain_height = generate_mountain_height(x, z);
        float hills_height = generate_hills_height(x, z);

        float blend_start = 0.3f;
        float blend_end = -0.2f;
        float blend = (erosion - blend_start) / (blend_end - blend_start);

        println("erosion: {}, blend factor: {}, height {}", erosion, blend, blend_heights(mountain_height, hills_height, blend));

        return blend_heights(hills_height, mountain_height, blend);
    }
    else
    {
        float hills_height = generate_hills_height(x, z);
        float plains_height = generate_plains_height(x, z);

        float blend = 1.0f - ((erosion - (-1.0f)) / (0.05f - (-1.0f)));

        return blend_heights(hills_height, plains_height, blend);
    }

    return sea_level;
}

float OverworldTerrainGenerator::blend_heights(float height1, float height2, float blend_factor)
{
    blend_factor = glm::clamp(blend_factor, 0.0f, 1.0f);
    blend_factor = glm::smoothstep(0.0f, 1.0f, blend_factor);
    float blended_heights = glm::mix(height1, height2, blend_factor);
    return glm::max(blended_heights, sea_level);
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
    ContinentalnessLevel c_level = get_continentalness_level(continentalness);

    float erosion = get_erosion_noise(x, z);
    uint32_t erosion_level = get_erosion_level(erosion);

    float pv = get_peaks_and_valleys_noise(x, z);
    PeaksAndValleysLevel pv_level = get_peaks_and_valleys_level(pv);

    if (c_level == ContinentalnessLevel::Ocean || c_level == ContinentalnessLevel::DeepOcean)
    {
        return TerrainShape::OCEAN;
    }

    if (c_level == ContinentalnessLevel::Coast)
    {
        return TerrainShape::BEACH;
    }

    if (c_level == ContinentalnessLevel::NearInland || c_level == ContinentalnessLevel::MidInland || c_level == ContinentalnessLevel::FarInland)
    {
        if (pv_level == PeaksAndValleysLevel::Valleys)
        {
            return TerrainShape::RIVER;
        }
        if (pv_level == PeaksAndValleysLevel::Low || pv_level == PeaksAndValleysLevel::Mid)
        {
            if (erosion_level < 4)
            {
                return TerrainShape::HILLS;
            }
            else
            {
                return TerrainShape::PLAINS;
            }
        }

        if (pv_level == PeaksAndValleysLevel::High || pv_level == PeaksAndValleysLevel::Peaks)
        {

            if (erosion_level <= 3)
            {
                return TerrainShape::MOUNTAINS;
            }
            else
            {
                return TerrainShape::HILLS;
            }
        }
    }
    return TerrainShape::PLAINS;
}

float OverworldTerrainGenerator::generate_beach_height(int64_t x, int64_t z)
{
    float beach_noise = m_noise.fractal(2, (float)x * 0.02f, 0.0f, (float)z * 0.02f);
    float coastal_slope = m_noise.fractal(1, (float)x * 0.0005f, 0.0f, (float)z * 0.0005f);

    return sea_level + beach_noise * 3.0f + coastal_slope * 4.0f;
}

float OverworldTerrainGenerator::generate_mountain_height(int64_t x, int64_t z)
{
    float large_peaks = m_noise.fractal(3, (float)x * 0.002f, 0.0f, (float)z * 0.002f);
    float height = sea_level + 40.0f + large_peaks * 60.0f;

    return std::min(height, 240.0f);
}

float OverworldTerrainGenerator::generate_hills_height(int64_t x, int64_t z)
{
    float hill_base = m_noise.fractal(3, (float)x * 0.006f, 0.0f, (float)z * 0.006f);
    float hill_detail = m_noise.fractal(4, (float)x * 0.015f, 0.0f, (float)z * 0.015f);

    return sea_level + hill_base * 10.0f + hill_detail * 10.0f;
}

float OverworldTerrainGenerator::generate_plains_height(int64_t x, int64_t z)
{
    float plains_base = m_noise.fractal(2, (float)x * 0.008f, 0.0f, (float)z * 0.008f);
    float gentle_rolls = m_noise.fractal(3, (float)x * 0.02f, 0.0f, (float)z * 0.02f);

    return sea_level + 8.0f + plains_base * 8.0f + gentle_rolls * 3.0f;
}

Biome OverworldTerrainGenerator::get_biome(BiomeNoise& biome_noise)
{
    Biome biome;
    auto continentalness_level = get_continentalness_level(biome_noise.continentalness);

    if (continentalness_level == ContinentalnessLevel::Ocean || continentalness_level == ContinentalnessLevel::DeepOcean)
    {
        biome = get_non_inland_biome(biome_noise);
    }
    else
    {
        biome = get_inland_biome(biome_noise);
    }

    return biome;
}

Biome OverworldTerrainGenerator::get_non_inland_biome(BiomeNoise& biome_noise)
{
    return Biome::Ocean;
}

Biome OverworldTerrainGenerator::get_inland_biome(BiomeNoise& biome_noise)
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
                return Biome::FrozenRiver;
            }
            return Biome::River;
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid)
        {
            if (erosion_level >= 0 && erosion_level <= 2)
            {
                return Biome::StonyShore;
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
            if (erosion_level == 6)
            {
                return get_beach_biome(temperature_level);
            }
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return Biome::JaggedPeaks;
                    }
                    return Biome::FrozenPeaks;
                }
                return Biome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }
    if (continentalness_level == ContinentalnessLevel::NearInland)
    {
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Valleys)
        {
            if (temperature_level == 0)
            {
                return Biome::FrozenRiver;
            }
            return Biome::River;
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid || peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return Biome::JaggedPeaks;
                    }
                    return Biome::FrozenPeaks;
                }
                return Biome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }
    if (continentalness_level == ContinentalnessLevel::MidInland || continentalness_level == ContinentalnessLevel::FarInland)
    {
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Valleys)
        {
            if (erosion_level >= 0 && erosion_level <= 1)
            {
                return get_middle_biome(temperature_level, humidity_level);
            }
            else
            {
                if (temperature_level == 0)
                {
                    return Biome::FrozenRiver;
                }
                return Biome::River;
            }
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            if (erosion_level == 0)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return Biome::JaggedPeaks;
                    }
                    return Biome::FrozenPeaks;
                }
                return Biome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level >= 0 && erosion_level <= 1)
            {
                if (temperature_level >= 0 && temperature_level <= 2)
                {
                    if (biome_noise.peaks_and_valleys < 0)
                    {
                        return Biome::JaggedPeaks;
                    }
                    return Biome::FrozenPeaks;
                }
                return Biome::StonyPeaks;
            }
            return get_middle_biome(temperature_level, humidity_level);
        }
    }

    return get_middle_biome(temperature_level, humidity_level);
}

Biome OverworldTerrainGenerator::get_beach_biome(uint32_t temperature)
{
    if (temperature == 4)
    {
        return Biome::Desert;
    }
    return Biome::Beach;
}

Biome OverworldTerrainGenerator::get_middle_biome(uint32_t temperature, uint32_t humidity)
{
    if (temperature == 0)
    {
        if (humidity == 4)
        {
            return Biome::Taiga;
        }
        return Biome::SnowyPlains;
    }
    if (temperature >= 1 && temperature <= 2)
    {
        if (humidity >= 0 && humidity <= 2)
        {
            return Biome::Plains;
        }
        return Biome::Taiga;
    }
    if (temperature == 3)
    {
        if (humidity >= 0 && humidity <= 1)
        {
            return Biome::Savanna;
        }
        return Biome::Plains;
    }
    return Biome::Desert;
}

ContinentalnessLevel OverworldTerrainGenerator::get_continentalness_level(float continentalness)
{
    if (continentalness >= -1.0f && continentalness < -0.455f)
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