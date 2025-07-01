#pragma once

#include "Core/Class.hpp"
#include "World/Generation/SimplexNoise.hpp"
#include <cstdint>

constexpr float base_height = 64.0f;

enum class NonInlandBiome
{
    FrozenOcean,
    Ocean,
    WarmOcean,
};

enum class InlandBiome
{
    FrozenRiver,
    River,
    StonyShore,
    StonyPeaks,
    JaggedPeaks,
    FrozenPeaks,
    // (Beach Biomes)
    Beach,
    Desert,
    // (Middle Biomes)
    SnowyPlains,
    Taiga,
    Plains,
    Savanna,

};
union Biome
{
    NonInlandBiome non_inland_biome;
    InlandBiome inland_biome;
};

struct BiomeNoise
{
    float continentalness;
    float erosion;
    float peaks_and_valleys;
    float temperature_noise;
    float humidity_noise;
};

class TerrainGenerator : public Object
{
    CLASS(TerrainGenerator, Object);

public:
    virtual bool has_block(int64_t x, int64_t y, int64_t z) = 0;
    virtual Biome get_biome(BiomeNoise& biome_noise) = 0;

    virtual float get_continentalness_noise(int64_t x, int64_t z) = 0;
    virtual float get_erosion_noise(int64_t x, int64_t z) = 0;
    virtual float get_peaks_and_valleys_noise(int64_t x, int64_t z) = 0;
    virtual float get_temperature_noise(int64_t x, int64_t z) = 0;
    virtual float get_humidity_noise(int64_t x, int64_t z) = 0;
};

class FlatTerrainGenerator : public TerrainGenerator
{
    CLASS(FlatTerrainGenerator, TerrainGenerator);

public:
    virtual bool has_block(int64_t x, int64_t y, int64_t z) override
    {
        (void)x;
        (void)z;
        return y < 10;
    }
};

enum class TerrainShape
{
    OCEAN,
    BEACH,
    PLAINS,
    MOUNTAINS,
    HILLS,
    DESERT,
};

enum class ContinentalnessLevel
{
    DeepOcean,
    Ocean,
    Coast,
    NearInland,
    MidInland,
    FarInland,
};

enum class PeaksAndValleysLevel
{
    Valleys,
    Low,
    Mid,
    High,
    Peaks,
};

class OverworldTerrainGenerator : public TerrainGenerator
{
    CLASS(OverworldTerrainGenerator, TerrainGenerator);

public:
    virtual bool has_block(int64_t x, int64_t y, int64_t z) override;
    virtual Biome get_biome(BiomeNoise& biome_noise) override;
    virtual float get_continentalness_noise(int64_t x, int64_t z) override;
    virtual float get_erosion_noise(int64_t x, int64_t z) override;
    virtual float get_peaks_and_valleys_noise(int64_t x, int64_t z) override;
    virtual float get_temperature_noise(int64_t x, int64_t z) override;
    virtual float get_humidity_noise(int64_t x, int64_t z) override;

    float get_height(int64_t x, int64_t z);
    float apply_pv_modification(float pv, float erosion);

    TerrainShape get_terrain_shape(int64_t x, int64_t z);

    float generate_ocean_height(int64_t x, int64_t z);
    float generate_beach_height(int64_t x, int64_t z);
    float generate_mountain_height(int64_t x, int64_t z);
    float generate_hills_height(int64_t x, int64_t z);
    float generate_plains_height(int64_t x, int64_t z);

    NonInlandBiome get_non_inland_biome(BiomeNoise& biome_noise);
    InlandBiome get_inland_biome(BiomeNoise& biome_noise);
    InlandBiome get_beach_biome(uint32_t temperature);
    InlandBiome get_middle_biome(uint32_t temperature, uint32_t humidity);

    ContinentalnessLevel get_continentalness_level(float continentalness);
    uint32_t get_erosion_level(float erosion);
    PeaksAndValleysLevel get_peaks_and_valleys_level(float peaks_and_valleys);
    uint32_t get_temperature_level(float temperature);
    uint32_t get_humidity_level(float humidity);

private:
    SimplexNoise m_noise;
};
