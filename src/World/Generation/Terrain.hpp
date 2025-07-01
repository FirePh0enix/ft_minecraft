#pragma once

#include "Core/Class.hpp"
#include "World/Generation/SimplexNoise.hpp"

class TerrainGenerator : public Object
{
    CLASS(TerrainGenerator, Object);

public:
    virtual bool has_block(int64_t x, int64_t y, int64_t z) = 0;
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

enum class TerrainType
{
    OCEAN,
    BEACH,
    PLAINS,
    MOUNTAINS,
    HILLS,
    DESERT,
};

class OverworldTerrainGenerator : public TerrainGenerator
{
    CLASS(OverworldTerrainGenerator, TerrainGenerator);

public:
    virtual bool has_block(int64_t x, int64_t y, int64_t z) override;

    float get_height(int64_t x, int64_t z);
    float get_terrain_height(TerrainType biome, int64_t x, int64_t z);
    float apply_pv_modification(float pv, float erosion);

    float get_continentalness_noise(int64_t x, int64_t z);
    float get_erosion_noise(int64_t x, int64_t z);
    float get_peaks_and_valleys_noise(int64_t x, int64_t z);
    float get_temperature_noise(int64_t x, int64_t z);
    float get_humidity_noise(int64_t x, int64_t z);

    TerrainType get_terrain_type(int64_t x, int64_t z);

    float generate_ocean_height(int64_t x, int64_t z);
    float generate_beach_height(int64_t x, int64_t z);
    float generate_mountain_height(int64_t x, int64_t z);
    float generate_hills_height(int64_t x, int64_t z);
    float generate_plains_height(int64_t x, int64_t z);

private:
    SimplexNoise m_noise;
};
