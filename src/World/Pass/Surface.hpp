#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Generator.hpp"

class SurfacePass : public GeneratorPass
{
    CLASS(SurfacePass, GeneratorPass);

public:
    SurfacePass();

    virtual BlockState process(const std::vector<BlockState>& previous_blocks, int64_t x, int64_t y, int64_t z) const override;
    virtual void seed_updated() override;

private:
    SimplexNoise m_simplex = SimplexNoise(0);

    uint16_t m_stone;
    uint16_t m_water;
};
