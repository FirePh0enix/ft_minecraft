#pragma once

#include "World/Generator.hpp"

class SurfacePass : public GeneratorPass
{
    CLASS(SurfacePass, GeneratorPass);

public:
    virtual BlockState process(GeneratorPass *previous_pass, int64_t x, int64_t y, int64_t z) const override;
};
