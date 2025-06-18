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

class OverworldTerrainGenerator : public TerrainGenerator
{
    CLASS(OverworldTerrainGenerator, TerrainGenerator);

public:
    // Return if a block should be placed.
    virtual bool has_block(int64_t x, int64_t y, int64_t z) override;
    float get_height(int64_t x, int64_t z);

private:
    SimplexNoise m_noise;
};
