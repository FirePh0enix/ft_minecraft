#include "Terrain.hpp"
#include "glm/common.hpp"
#include <array>
#include <cstdint>
#include <print>
#include <utility>

bool OverworldTerrainGenerator::has_block(int64_t x, int64_t y, int64_t z)
{
    // Use multiple 2D FBM noise and splines points to calculate a height.
    float expected_height = get_height(x, z);

    if ((float)y <= sea_level && (float)y < expected_height)
    {
        return true;
    }

    // The more we release the squash factor, the more it seems strange and chaotic but also creating cliffs and floating island.
    float squash_factor = 0.0009f;
    float noise = get_3d_noise(x, y, z);
    float density = noise + (expected_height - (float)y) * squash_factor;
    density = density * 1.2f;

    // Negative density means it's air.
    return density > 0.0f;
}

float OverworldTerrainGenerator::get_3d_noise(int64_t x, int64_t y, int64_t z)
{
    return m_noise.fractal(8, (float)x * 0.00052f, (float)y * 0.0025f, (float)z * 0.00052f);
}

float OverworldTerrainGenerator::get_height(int64_t x, int64_t z)
{
    float continentalness = get_continentalness_noise(x, z);
    float erosion = get_erosion_noise(x, z);
    float peaks_and_valleys = get_peaks_and_valleys_noise(x, z);

    float base_height = get_continentalness_spline(continentalness);
    float erosion_factor = get_erosion_spline(erosion);

    float sea_difference = base_height - sea_level;
    float normal_terrain = sea_level + (sea_difference * erosion_factor);

    BiomeNoise noise{
        .continentalness = continentalness,
        .erosion = erosion,
        .peaks_and_valleys = peaks_and_valleys,
        .temperature_noise = get_temperature_noise(x, z),
        .humidity_noise = get_humidity_noise(x, z),
    };

    Biome biome = get_biome(noise);

    // Ensure that i'm not applying any erosion or peaks and valleys to water, else we will have not flat water.
    if (biome == Biome::Ocean || biome == Biome::River || biome == Biome::FrozenRiver)
    {
        return sea_level;
    }

    // Since they are close to water, the transition was not always smooth.
    // if (biome == Biome::Beach || biome == Biome::StonyShore)
    // {
    //     // Create details and make the stairs like transitions.
    //     float detail_noise = m_noise.fractal(3, (float)x * 0.1f, 5000.0f, (float)z * 0.1f) * 1.0f;
    //     float beach_height = sea_level + 2.0f + detail_noise;

        //        if (continentalness >= -1.0f && continentalness < -0.455f)
        // {
        //     return ContinentalnessLevel::DeepOcean;
        // }
        // else if (continentalness >= -0.455f && continentalness < -0.19f)
        // {
        //     return ContinentalnessLevel::Ocean;
        // }
        // else if (continentalness >= -0.19f && continentalness < -0.11f)
        // {
        //     return ContinentalnessLevel::Coast;
        // }
        // else if (continentalness >= -0.11f && continentalness < 0.03f)
        // {
        //     return ContinentalnessLevel::NearInland;
        // }
        // else if (continentalness >= 0.03f && continentalness < 0.3f)
        // {
        //     return ContinentalnessLevel::MidInland;
        // }
        // return ContinentalnessLevel::FarInland;

        // if (continentalness < -0.11f)
        // {
        //     float blend = (continentalness + 0.19f) / 0.04f;
        //     return glm::mix(sea_level, beach_height, blend);
        // }

        // return beach_height;
    // }

    float water_proximity = 0.0f;

    if (continentalness >= -0.19f && continentalness < 0.0f)
    {
        water_proximity = (continentalness + 0.19f) / 0.19f;
    }
    else
    {
        water_proximity = 1.0f;
    }

    if (peaks_and_valleys >= -0.85f && peaks_and_valleys < -0.2f)
    {
        float detail_noise = m_noise.fractal(3, (float)x * 0.05f, 0.0f, (float)z * 0.05f) * 2.0f;

        if (peaks_and_valleys < -0.7f)
        {
            float height = sea_level + 3.0f + detail_noise;
            float blend = (peaks_and_valleys + 0.85f) / 0.15f;
            float result = glm::mix(sea_level, height, blend);

            if (water_proximity < 1.0f)
            {
                float max_height = glm::min(height, sea_level + 5.0f * water_proximity);
                return glm::mix(sea_level, max_height, blend * water_proximity);
            }

            return result;
        }
        else if (peaks_and_valleys < -0.55f)
        {
            float height = sea_level + 5.0f + detail_noise;

            if (water_proximity < 1.0f)
            {
                return glm::mix(sea_level, height, water_proximity);
            }

            return height;
        }
        else if (peaks_and_valleys < -0.4f)
        {
            float height = sea_level + 8.0f + detail_noise;

            if (water_proximity < 1.0f)
            {
                return glm::mix(sea_level + 2.0f, height, water_proximity);
            }

            return height;
        }
        else
        {
            float height = sea_level + 12.0f + detail_noise;
            float blend = (peaks_and_valleys + 0.4f) / 0.2f;
            float peaks_valleys_height = get_peaks_and_valleys_spline(peaks_and_valleys);
            float normal_with_pv = normal_terrain + peaks_valleys_height;

            if (water_proximity < 1.0f)
            {
                float max_height = sea_level + 3.0f + (9.0f * water_proximity);
                float adjusted_normal = glm::mix(sea_level + 3.0f, normal_with_pv, water_proximity);
                return glm::mix(max_height, adjusted_normal, blend * water_proximity);
            }

            return glm::mix(height, normal_with_pv, blend);
        }
    }

    float peaks_valleys_height = get_peaks_and_valleys_spline(peaks_and_valleys);
    normal_terrain += peaks_valleys_height;

    if (water_proximity < 1.0f)
    {
        float water_edge_height = sea_level + 3.0f;
        float target_height = glm::mix(water_edge_height, normal_terrain, water_proximity);
        return target_height;
    }

    return glm::max(sea_level, normal_terrain);
}

float OverworldTerrainGenerator::get_continentalness_spline(float continentalness)
{
    const std::array<std::pair<float, float>, 7> spline_points = {
        std::pair<float, float>{-1.000f, sea_level}, // DeepOcean
        std::pair<float, float>{-0.455f, sea_level}, // Ocean
        std::pair<float, float>{-0.19f, sea_level},  // Coast
        std::pair<float, float>{-0.11f, 64.0f},      // NearInland
        std::pair<float, float>{0.03f, 75.0f},       // MidInland
        std::pair<float, float>{0.3f, 90.0f},        // FarInland
        std::pair<float, float>{1.0f, 120.0f}        // Extreme peaks
    };

    for (size_t i = 0; i < spline_points.size() - 1; i++)
    {
        if (continentalness >= spline_points[i].first && continentalness <= spline_points[i + 1].first)
        {
            float t = (continentalness - spline_points[i].first) /
                      (spline_points[i + 1].first - spline_points[i].first);

            return glm::mix(spline_points[i].second, spline_points[i + 1].second, t);
        }
    }

    if (continentalness < spline_points[0].first)
        return spline_points[0].second;
    return spline_points[spline_points.size() - 1].second;
}

float OverworldTerrainGenerator::get_erosion_spline(float erosion)
{
    const std::array<std::pair<float, float>, 8> spline_points = {
        std::pair<float, float>{-1.00f, 1.8f},   // Level 0: Very rugged
        std::pair<float, float>{-0.78f, 1.5f},   // Level 1: More dramatic
        std::pair<float, float>{-0.375f, 1.2f},  // Level 2: Slightly increased
        std::pair<float, float>{-0.2225f, 1.0f}, // Level 3: Neutral
        std::pair<float, float>{0.05f, 0.9f},    // Level 4: Slightly smooth
        std::pair<float, float>{0.45f, 0.7f},    // Level 5: Smooth
        std::pair<float, float>{0.55f, 0.5f},    // Level 6: Very smooth
        std::pair<float, float>{1.00f, 0.5f}     // End point
    };

    for (size_t i = 0; i < spline_points.size() - 1; i++)
    {
        if (erosion >= spline_points[i].first && erosion <= spline_points[i + 1].first)
        {
            float t = (erosion - spline_points[i].first) /
                      (spline_points[i + 1].first - spline_points[i].first);

            return glm::mix(spline_points[i].second, spline_points[i + 1].second, t);
        }
    }

    if (erosion < spline_points[0].first)
        return spline_points[0].second;
    return spline_points[spline_points.size() - 1].second;
}
float OverworldTerrainGenerator::get_peaks_and_valleys_spline(float peaks_and_valleys)
{
    const std::array<std::pair<float, float>, 7> spline_points = {
        std::pair<float, float>{-1.0f, -50.0f}, // Deeper valleys
        std::pair<float, float>{-0.6f, -25.0f}, // Valley
        std::pair<float, float>{-0.2f, -8.0f},  // Slight depression
        std::pair<float, float>{0.0f, 0.0f},    // Neutral
        std::pair<float, float>{0.2f, 10.0f},   // Small bump (bigger)
        std::pair<float, float>{0.6f, 35.0f},   // Hill/peak (bigger)
        std::pair<float, float>{1.0f, 70.0f}    // Major peak (much bigger)
    };

    for (size_t i = 0; i < spline_points.size() - 1; i++)
    {
        if (peaks_and_valleys >= spline_points[i].first && peaks_and_valleys <= spline_points[i + 1].first)
        {
            float t = (peaks_and_valleys - spline_points[i].first) /
                      (spline_points[i + 1].first - spline_points[i].first);

            return glm::mix(spline_points[i].second, spline_points[i + 1].second, t);
        }
    }

    if (peaks_and_valleys < spline_points[0].first)
        return spline_points[0].second;
    return spline_points[spline_points.size() - 1].second;
}

float OverworldTerrainGenerator::get_continentalness_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(16, (float)x * 0.00052f, 0.0f, (float)z * 0.00052f);
}

float OverworldTerrainGenerator::get_erosion_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(8, (float)x * 0.001f, 1000.0f, (float)z * 0.001f);
}

float OverworldTerrainGenerator::get_peaks_and_valleys_noise(int64_t x, int64_t z)
{
    float weirdness = m_noise.fractal(8, (float)x * 0.0025f, 2000.0f, (float)z * 0.0025f);
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
            if (erosion_level <= 2)
            {
                return Biome::StonyShore;
            }
            if (erosion_level <= 4)
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
        if (peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level <= 2)
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
            if (erosion_level <= 5)
            {
                if (temperature_level == 0)
                {
                    return Biome::FrozenRiver;
                }
                return Biome::River;
            }
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Low || peaks_and_valleys_level == PeaksAndValleysLevel::Mid || peaks_and_valleys_level == PeaksAndValleysLevel::High)
        {
            return get_middle_biome(temperature_level, humidity_level);
        }
        if (peaks_and_valleys_level == PeaksAndValleysLevel::Peaks)
        {
            if (erosion_level == 0)
            {
                if (temperature_level <= 2)
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
            if (erosion_level <= 1)
            {
                return get_middle_biome(temperature_level, humidity_level);
            }
            else
            {
                if (erosion_level <= 5)
                {
                    if (temperature_level == 0)
                    {
                        return Biome::FrozenRiver;
                    }
                    return Biome::River;
                }
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
                if (temperature_level <= 2)
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
            if (erosion_level <= 1)
            {
                if (temperature_level <= 2)
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
    else if (continentalness >= -0.11f && continentalness < 0.03f)
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