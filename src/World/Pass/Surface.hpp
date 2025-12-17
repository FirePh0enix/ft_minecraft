#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Generator.hpp"

class SurfacePass : public GeneratorPass
{
    CLASS(SurfacePass, GeneratorPass);

public:
    SurfacePass();

    virtual void process(int64_t x, int64_t z, int64_t cx, int64_t cz, std::vector<BlockState>& blocks) const override;
    virtual void seed_updated() override;

private:
    SimplexNoise m_simplex = SimplexNoise(0);

    uint16_t m_stone;
    uint16_t m_water;
};
