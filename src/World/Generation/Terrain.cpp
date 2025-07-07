#include "Terrain.hpp"
#include "Core/Print.hpp"
#include "glm/common.hpp"
#include <array>
#include <cstdint>
#include <print>
#include <utility>

/**
 * @brief Determines whether a block exists at the given world position using a density function.
 * @details Combines 2D height mapping with 3D noise to create terrain with features like overhangs
 *          and caves. Uses a density threshold approach where positive density values indicate solid
 *          blocks and negative values indicate air.
 * @param x The x-coordinate in world space
 * @param y The y-coordinate (height) in world space
 * @param z The z-coordinate in world space
 * @return true if a solid block exists at this position, false if it's air
 * @see get_height For the 2D height calculation based on multiple noise functions
 * @see get_3d_noise For the 3D noise generation used to create terrain features
 */
bool OverworldTerrainGenerator::has_block(int64_t x, int64_t y, int64_t z)
{
    // Use multiple 2D FBM noise and splines points to calculate a height.
    float expected_height = get_height(x, z);
    constexpr float threshold = 0.05f;

    float spaghetti_cave = glm::abs(get_spaghetti_cave_noise(x, y, z));
    float cheese_cave = get_cheese_cave_noise(x, y, z);

    BiomeNoise biome_noise{
        .continentalness = get_continentalness_noise(x, z),
        .erosion = get_erosion_noise(x, z),
        .peaks_and_valleys = get_peaks_and_valleys_noise(x, z),
        .temperature_noise = get_temperature_noise(x, z),
        .humidity_noise = get_humidity_noise(x, z),
    };

    Biome biome = get_biome(biome_noise);

    // Ensure that even the surface of water has a block.
    if (((biome == Biome::Ocean || biome == Biome::River) && ((float)y == sea_level)) || y == 0)
    {
        return true;
    }

    if ((spaghetti_cave < threshold || cheese_cave > 0.38f))
    {
        return false;
    }

    if ((float)y <= sea_level && (float)y <= expected_height)
    {
        return true;
    }

    // The more we release the squash factor, the more it seems strange and chaotic but also creating cliffs and floating island.
    float squash_factor = 0.0009f;
    float noise = get_3d_noise(x, y, z);
    float density = noise + (expected_height - (float)y) * squash_factor;

    return density > 0.0f;
}

float OverworldTerrainGenerator::get_3d_noise(int64_t x, int64_t y, int64_t z)
{
    return m_noise.fractal(8, (float)x * 0.00052f, (float)y * 0.0025f, (float)z * 0.00052f);
}

float OverworldTerrainGenerator::get_spaghetti_cave_noise(int64_t x, int64_t y, int64_t z)
{
    return m_noise.fractal(6, (float)x * 0.04f, (float)y * 0.04f, (float)z * 0.04f);
}

float OverworldTerrainGenerator::get_cheese_cave_noise(int64_t x, int64_t y, int64_t z)
{
    return m_noise.fractal(5, (float)x * 0.01f, (float)y * 0.01f, (float)z * 0.01f);
}

/**
 * @brief Calculates terrain height using multiple 2D noise functions and spline interpolation.
 * @details Combines continentalness, erosion, and peaks & valleys noise to generate a terrain
 *          height value. Special handling ensures water bodies remain flat at sea level.
 *          The process involves:
 *          1. Generating base noise values
 *          2. Applying spline interpolation to get height components
 *          3. Ensuring water biomes are flat at sea level
 *
 * @param x x-coordinate in world space
 * @param z z-coordinate in world space
 * @return Terrain height value at the specified (x,z) position
 *
 * @see get_continentalness_noise For continental shape generation
 * @see get_erosion_noise For terrain erosion patterns
 * @see get_peaks_and_valleys_noise For local height variations
 * @see get_biome For biome determination based on combined noise values
 */
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

    float peaks_valleys_height = get_peaks_and_valleys_spline(peaks_and_valleys);
    normal_terrain += peaks_valleys_height;

    return glm::max(sea_level, normal_terrain);
}

/**
 * @brief Maps continentalness noise values to terrain base heights using spline interpolation.
 * @details Uses a predefined set of control points to convert continentalness values (-1.0 to 1.0)
 *          into appropriate terrain heights. Lower values generate ocean depths, while higher
 *          values create inland terrain with extreme peaks at maximum values. Linear interpolation
 *          is performed between control points to ensure smooth transitions.
 *
 * @param continentalness A noise value between -1.0 and 1.0 representing continental shapes
 * @return The interpolated base height corresponding to the given continentalness value
 *
 * @see get_continentalness_level For classification of continentalness into discrete zones
 * @see get_height For how this function is used in the overall height calculation
 */
float OverworldTerrainGenerator::get_continentalness_spline(float continentalness)
{
    const std::array<std::pair<float, float>, 7> spline_points = {
        std::pair<float, float>{-1.000f, sea_level}, // DeepOcean
        std::pair<float, float>{-0.455f, sea_level}, // Ocean
        std::pair<float, float>{-0.19f, sea_level},  // Coast
        std::pair<float, float>{-0.11f, 64.0f},      // NearInland
        std::pair<float, float>{0.03f, 75.0f},       // MidInland
        std::pair<float, float>{0.3f, 90.0f},        // FarInland
        std::pair<float, float>{1.0f, 200.0f}        // Extreme peaks
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

/**
 * @brief Maps erosion noise values to terrain modifiers using spline interpolation.
 * @details Converts erosion values (-1.0 to 1.0) into terrain modification factors that
 *          control how dramatically the continentalness affects the final height. Lower
 *          erosion values produce more dramatic, rugged terrain with amplified height
 *          differences, while higher values create smoother, more eroded landscapes with
 *          reduced height variation.
 *
 * @param erosion A noise value between -1.0 and 1.0 representing terrain erosion patterns
 * @return A multiplier (0.5-1.8) that will be applied to height differences from sea level
 *
 * @see get_erosion_level For classification of erosion into discrete levels
 * @see get_height For how this erosion factor is applied in terrain generation
 */
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

/**
 * @brief Maps peaks and valleys noise values to terrain height offsets using spline interpolation.
 * @details Converts peaks and valleys values (-1.0 to 1.0) into height adjustments that create
 *          local terrain features like valleys, depressions, hills, and peaks. Negative noise
 *          values generate depressions below the base terrain level, while positive values
 *          create elevated features. These offsets are applied after the base terrain height
 *          (determined by continentalness and erosion) has been calculated.
 *
 * @param peaks_and_valleys A noise value between -1.0 and 1.0 representing local terrain variations
 * @return A height offset (between -75.0 and 75.0) that will be added to the base terrain height
 *
 * @see get_peaks_and_valleys_level For classification of terrain features into discrete categories
 * @see get_height For how this offset is applied in the overall terrain generation process
 */
float OverworldTerrainGenerator::get_peaks_and_valleys_spline(float peaks_and_valleys)
{
    const std::array<std::pair<float, float>, 7> spline_points = {
        std::pair<float, float>{-1.0f, -75.0f},  // Deeper valleys
        std::pair<float, float>{-0.85f, -25.0f}, // Valley
        std::pair<float, float>{-0.2f, -8.0f},   // Slight depression
        std::pair<float, float>{0.0f, 0.0f},     // Neutral
        std::pair<float, float>{0.2f, 8.0f},     // Small bump
        std::pair<float, float>{0.85f, 25.0f},   // Hill/peak
        std::pair<float, float>{1.0f, 75.0f}     // Major peak
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

/**
 * @brief Generates continentalness noise at the specified world position.
 * @details Produces a noise value in the range [-1.0, 1.0] that defines large-scale
 *          terrain features like oceans, coastlines, and inland areas. Uses fractal
 *          Perlin/Simplex noise with 16 octaves for smooth, natural-looking transitions.
 *          Lower frequency values (0.00052) ensure these features span large areas.
 *
 * @param x x-coordinate in world space (block position)
 * @param z z-coordinate in world space (block position)
 * @return A noise value between -1.0 and 1.0, where negative values typically represent
 *         oceans and positive values represent continents
 *
 * @see https://minecraft.fandom.com/wiki/Noise_generator
 * @see get_continentalness_spline For how this noise value is mapped to terrain heights
 */
float OverworldTerrainGenerator::get_continentalness_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(16, (float)x * 0.00052f, 0.0f, (float)z * 0.00052f);
}

/**
 * @brief Generates erosion noise at the specified world position.
 * @details Produces a noise value in the range [-1.0, 1.0] that controls terrain roughness
 *          and feature definition. Uses fractal noise with 8 octaves at medium frequency
 *          (0.001) to create medium-scale terrain variations. Higher erosion values create
 *          smoother terrain, while lower values produce more rugged landscapes with
 *          pronounced features.
 *
 * @param x x-coordinate in world space (block position)
 * @param z z-coordinate in world space (block position)
 * @return A noise value between -1.0 and 1.0 representing terrain erosion patterns
 *
 * @see https://minecraft.fandom.com/wiki/Noise_generator
 * @see get_erosion_spline For how this noise value affects terrain smoothness
 * @see get_height For how erosion is used in terrain generation
 */
float OverworldTerrainGenerator::get_erosion_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(8, (float)x * 0.001f, 1000.0f, (float)z * 0.001f);
}

/**
 * @brief Generates peaks and valleys noise using a weirdness transformation formula.
 * @details Creates local terrain variations by transforming fractal noise through the formula:
 *          PV = 1.0 - |3.0 * |weirdness| - 2.0|
 *
 *          This transformation creates a pattern where noise values naturally form alternating
 *          peaks (positive values) and valleys (negative values) with smooth transitions.
 *          Uses higher frequency (0.0025) to create smaller-scale terrain features than
 *          continentalness or erosion.
 *
 * @param x x-coordinate in world space (block position)
 * @param z z-coordinate in world space (block position)
 * @return A noise value between -1.0 and 1.0 where positive values represent peaks and
 *         negative values represent valleys
 *
 * @see https://minecraft.wiki/w/World_generation
 * @see get_peaks_and_valleys_spline For how this noise is mapped to height adjustments
 * @see get_height For how these values affect the final terrain generation
 */
float OverworldTerrainGenerator::get_peaks_and_valleys_noise(int64_t x, int64_t z)
{
    float weirdness = m_noise.fractal(8, (float)x * 0.0025f, 2000.0f, (float)z * 0.0025f);
    float abs_weirdness = std::abs(weirdness);
    float pv = 1.0f - std::abs(3.0f * abs_weirdness - 2.0f);
    return std::clamp(pv, -1.0f, 1.0f);
}

/**
 * @brief Generates temperature noise at the specified world position.
 * @details Produces a noise value in the range [-1.0, 1.0] that defines climate zones
 *          across the world. Uses fractal noise with 4 octaves at low frequency (0.0002)
 *          to create large, gradual temperature transitions across the landscape. This
 *          noise is primarily used for biome determination, with lower values generally
 *          representing colder biomes and higher values warmer biomes.
 *
 * @param x x-coordinate in world space (block position)
 * @param z z-coordinate in world space (block position)
 * @return A noise value between -1.0 and 1.0 representing temperature variations
 *
 * @see get_temperature_level For how this value is classified into discrete temperature zones
 * @see get_biome For how temperature affects biome selection
 */
float OverworldTerrainGenerator::get_temperature_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(4, (float)x * 0.0002f, 0.0f, (float)z * 0.0002f);
}

/**
 * @brief Generates humidity noise at the specified world position.
 * @details Produces a noise value in the range [-1.0, 1.0] that defines moisture levels
 *          across the world. Uses fractal noise with 4 octaves at low frequency (0.0003)
 *          to create large, gradual humidity transitions. The frequency is slightly higher
 *          than temperature noise, creating moisture patterns that vary independently from
 *          temperature zones. This noise primarily affects biome selection, with lower values
 *          typically representing drier biomes and higher values wetter biomes.
 *
 * @param x x-coordinate in world space (block position)
 * @param z z-coordinate in world space (block position)
 * @return A noise value between -1.0 and 1.0 representing humidity variations
 *
 * @see get_humidity_level For how this value is classified into discrete moisture categories
 * @see get_biome For how humidity combines with temperature to determine biomes
 */
float OverworldTerrainGenerator::get_humidity_noise(int64_t x, int64_t z)
{
    return m_noise.fractal(4, (float)x * 0.0003f, 0.0f, (float)z * 0.0003f);
}

/**
 * @brief Determines the appropriate biome based on multiple noise parameters.
 * @details Uses a multi-step process to select biomes based on the combination of five noise values:
 *          1. First determines if the location is oceanic or inland based on continentalness
 *          2. Then applies different biome selection logic depending on this classification
 *          3. For oceanic areas, calls get_non_inland_biome() for water-based biomes
 *          4. For inland areas, calls get_inland_biome() for land-based biomes
 *
 *          This approach creates natural biome transitions and ensures appropriate biome
 *          placement based on the terrain's fundamental characteristics.
 *
 * @param biome_noise A structure containing five noise values (continentalness, erosion,
 *                    peaks_and_valleys, temperature_noise, and humidity_noise)
 * @return The selected Biome enum value for the given location
 *
 * @see get_continentalness_level For how continentalness is classified
 * @see get_non_inland_biome For oceanic biome selection logic
 * @see get_inland_biome For land-based biome selection logic
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Determines the appropriate water-based biome based on noise parameters.
 * @details Selects a non-inland (water) biome type based on the provided noise values.
 *          Non-inland biomes include oceans, deep oceans, frozen oceans, and other
 *          water-based biome types. In a complete implementation, this would consider:
 *          - Temperature noise for determining frozen vs. normal oceans
 *          - Continentalness for deep ocean vs. regular ocean determination
 *          - Other noise values for special ocean variants (warm ocean, etc.)
 *
 *          Note: Current implementation is simplified and returns Ocean for all inputs.
 *
 * @param biome_noise A structure containing five noise values (continentalness, erosion,
 *                    peaks_and_valleys, temperature_noise, and humidity_noise)
 * @return A water-based Biome enum value appropriate for the location
 *
 * @see get_biome For the main biome selection logic that calls this function
 * @see get_inland_biome For the counterpart that handles land-based biomes
 * @see https://minecraft.wiki/w/World_generation
 */
//  TODO: Add more non inland biomes.
Biome OverworldTerrainGenerator::get_non_inland_biome(BiomeNoise& biome_noise)
{
    return Biome::Ocean;
}

/**
 * @brief Determines the appropriate land-based biome using a complex classification system.
 * @details Selects inland biomes through a multi-tier decision tree that considers:
 *          - Continentalness level (Coast, NearInland, MidInland, FarInland)
 *          - Peaks and valleys level (Valleys, Low, Mid, High, Peaks)
 *          - Temperature level (0-4, from coldest to hottest)
 *          - Erosion level (0-6, from most rugged to smoothest)
 *          - Humidity level (0-4, from driest to wettest)
 *
 *          Special biome placements include:
 *          - Rivers in valley areas
 *          - Beaches and stony shores along coastlines
 *          - Mountain peaks (frozen, jagged, stony) in high-elevation areas
 *          - Standard biomes (forest, plains, desert, etc.) in middle regions
 *
 *          This sophisticated approach creates natural-looking biome transitions that
 *          follow realistic terrain patterns.
 *
 * @param biome_noise A structure containing the five noise values used for biome determination
 * @return An inland Biome enum value appropriate for the location
 *
 * @see get_biome For the main function that calls this method
 * @see get_middle_biome For the selection of standard terrain biomes
 * @see get_beach_biome For the selection of beach variants
 * @see get_continentalness_level For continentalness classification
 * @see get_peaks_and_valleys_level For terrain feature classification
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Determines the appropriate beach biome based on temperature level.
 * @details Selects the beach variant that corresponds to the provided temperature level.
 *          For most temperature levels (0-3), a standard Beach biome is returned.
 *          For the hottest temperature level (4), Desert is returned instead, as hot
 *          regions typically have sandy shorelines that blend into desert biomes.
 *
 *          Temperature levels represent climate zones:
 *          - 0: Frozen/Snowy
 *          - 1: Cold
 *          - 2: Temperate/Mild
 *          - 3: Warm
 *          - 4: Hot
 *
 * @param temperature An unsigned integer (0-4) representing the temperature level
 * @return The appropriate beach-type Biome enum value (Beach or Desert)
 *
 * @see get_temperature_level For how temperature noise is converted to discrete levels
 * @see get_inland_biome For the function that calls this method
 * @see https://minecraft.wiki/w/World_generation
 */
Biome OverworldTerrainGenerator::get_beach_biome(uint32_t temperature)
{
    if (temperature == 4)
    {
        return Biome::Desert;
    }
    return Biome::Beach;
}

/**
 * @brief Determines the standard terrain biome based on temperature and humidity levels.
 * @details Selects appropriate "middle" biomes (standard terrain biomes that make up the
 *          majority of the world) based on climate parameters. The biome selection follows
 *          ecological patterns where:
 *
 *          Temperature levels (0-4):
 *          - 0: Frozen/Snowy regions → SnowyPlains or Taiga
 *          - 1-2: Temperate regions → Plains or Taiga
 *          - 3: Warm regions → Savanna or Plains
 *          - 4: Hot regions → Desert
 *
 *          Humidity levels (0-4) modify these selections:
 *          - Higher humidity in cold areas → Taiga instead of SnowyPlains
 *          - Higher humidity in temperate areas → Taiga instead of Plains
 *          - Lower humidity in warm areas → Savanna instead of Plains
 *          - Humidity has no effect in hot areas (always Desert)
 *
 * @param temperature An unsigned integer (0-4) representing the temperature level
 * @param humidity An unsigned integer (0-4) representing the humidity/moisture level
 * @return The appropriate standard Biome enum value for the given climate
 *
 * @see get_temperature_level For how temperature noise is converted to discrete levels
 * @see get_humidity_level For how humidity noise is converted to discrete levels
 * @see get_inland_biome For the function that calls this method
 * @see https://minecraft.wiki/w/World_generation
 */
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
    if (temperature <= 2)
    {
        if (humidity <= 2)
        {
            return Biome::Plains;
        }
        return Biome::Taiga;
    }
    if (temperature == 3)
    {
        if (humidity <= 1)
        {
            return Biome::Savanna;
        }
        return Biome::Plains;
    }
    return Biome::Desert;
}

/**
 * @brief Classifies a continentalness noise value into a discrete terrain level.
 * @details Converts the continuous noise value into one of six terrain categories using
 *          specific thresholds. These levels represent the gradient from deep ocean to
 *          far inland, with precise boundaries that create natural-looking transitions:
 *
 *          - DeepOcean:   -1.000 to -0.455 (deepest underwater terrain)
 *          - Ocean:       -0.455 to -0.190 (standard underwater terrain)
 *          - Coast:       -0.190 to -0.110 (shorelines and beaches)
 *          - NearInland:  -0.110 to  0.030 (coastal plains and lowlands)
 *          - MidInland:    0.030 to  0.300 (standard inland terrain)
 *          - FarInland:    0.300 to  1.000 (elevated inland terrain)
 *
 * @param continentalness A float between -1.0 and 1.0 representing the continentalness noise value
 * @return The appropriate ContinentalnessLevel enum value for the given noise
 *
 * @see get_continentalness_noise For how the input noise value is generated
 * @see get_biome For how continentalness level affects biome selection
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Classifies an erosion noise value into a discrete terrain roughness level.
 * @details Converts the continuous erosion noise value into one of seven terrain roughness
 *          categories (0-6) using precise thresholds. Erosion levels represent the
 *          degree of terrain roughness, with lower levels creating more dramatic and rugged
 *          terrain features, while higher levels produce smoother landscapes:
 *
 *          - Level 0: -1.000 to -0.780 (extremely rugged terrain, mountain peaks, badlands)
 *          - Level 1: -0.780 to -0.375 (highly rugged terrain, cliff formations)
 *          - Level 2: -0.375 to -0.223 (moderately rugged, hills and ravines)
 *          - Level 3: -0.223 to  0.050 (slight roughness, rolling hills)
 *          - Level 4:  0.050 to  0.450 (mostly smooth terrain with gentle variations)
 *          - Level 5:  0.450 to  0.550 (very smooth terrain, plains-like)
 *          - Level 6:  0.550 to  1.000 (completely smooth terrain, flat areas)
 *
 * @param erosion A float between -1.0 and 1.0 representing the erosion noise value
 * @return An unsigned integer (0-6) representing the erosion level
 *
 * @see get_erosion_noise For how the input noise value is generated
 * @see get_inland_biome For how erosion level affects biome placement
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Classifies a peaks and valleys noise value into a discrete terrain elevation level.
 * @details Converts the continuous peaks and valleys noise value into one of five terrain
 *          elevation categories using precise thresholds. These levels represent the local
 *          terrain variation from valleys to peaks, independent of the overall continentalness:
 *
 *          - Valleys: -1.000 to -0.850 (lowest points, riverbeds, ravines)
 *          - Low:     -0.850 to -0.200 (low-lying areas, floodplains)
 *          - Mid:     -0.200 to  0.200 (average terrain height, flatlands)
 *          - High:     0.200 to  0.700 (elevated terrain, hills, plateaus)
 *          - Peaks:    0.700 to  1.000 (highest points, mountain peaks)
 *
 *          These classifications primarily affect local terrain height variations and
 *          influence biome placement for terrain-specific biomes like rivers,
 *          mountains, and plateaus.
 *
 * @param peaks_and_valleys A float between -1.0 and 1.0 representing the local terrain variation
 * @return The appropriate PeaksAndValleysLevel enum value for the given noise
 *
 * @see get_peaks_and_valleys_noise For how the input noise value is generated
 * @see get_inland_biome For how peaks and valleys levels affect biome selection
 * @see get_biome For the overall biome determination process
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Classifies a temperature noise value into a discrete climate level.
 * @details Converts the continuous temperature noise value into one of five climate
 *          categories (0-4) using precise thresholds. These levels represent the
 *          gradient from coldest to hottest regions in the world:
 *
 *          - Level 0: -1.000 to -0.450 (frozen/snowy regions)
 *          - Level 1: -0.450 to -0.150 (cold regions)
 *          - Level 2: -0.150 to  0.200 (temperate/mild regions)
 *          - Level 3:  0.200 to  0.550 (warm regions)
 *          - Level 4:  0.550 to  1.000 (hot regions)
 *
 *          Temperature level is a primary factor in biome determination,
 *          affecting vegetation types, snow coverage, and overall ecosystem
 *          characteristics.
 *
 * @param temperature A float between -1.0 and 1.0 representing the temperature noise value
 * @return An unsigned integer (0-4) representing the temperature level
 *
 * @see get_temperature_noise For how the input noise value is generated
 * @see get_middle_biome For how temperature level affects standard biome selection
 * @see get_beach_biome For how temperature level affects coastal biome selection
 * @see https://minecraft.wiki/w/World_generation
 */
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

/**
 * @brief Classifies a humidity noise value into a discrete moisture level.
 * @details Converts the continuous humidity noise value into one of five moisture
 *          categories (0-4) using precise thresholds. These levels represent the
 *          gradient from driest to wettest regions in the world:
 *
 *          - Level 0: -1.000 to -0.350 (very dry regions, deserts)
 *          - Level 1: -0.350 to -0.100 (dry regions, savannas)
 *          - Level 2: -0.100 to  0.100 (moderate moisture, plains)
 *          - Level 3:  0.100 to  0.300 (moist regions, forests)
 *          - Level 4:  0.300 to  1.000 (very wet regions, swamps)
 *
 *          Humidity level works in conjunction with temperature to determine
 *          vegetation density, biome types, and overall ecosystem characteristics.
 *          Higher humidity typically results in more lush and densely vegetated biomes.
 *
 * @param humidity A float between -1.0 and 1.0 representing the humidity noise value
 * @return An unsigned integer (0-4) representing the humidity level
 *
 * @see get_humidity_noise For how the input noise value is generated
 * @see get_middle_biome For how humidity level affects biome selection
 * @see get_inland_biome For the broader biome determination process
 * @see https://minecraft.wiki/w/World_generation
 */
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