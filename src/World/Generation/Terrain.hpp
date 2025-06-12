#pragma once

class FlatTerrainGenerator
{
public:
    FlatTerrainGenerator()
    {
    }

    static bool has_block(int64_t x, int64_t y, int64_t z)
    {
        (void)x;
        (void)z;
        return y < 10;
    }
};
