#pragma once

#include "Core/Class.hpp"

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
