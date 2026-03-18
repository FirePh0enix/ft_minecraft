#pragma once

#include "World/Generator.hpp"

class FlatSurfacePass : public GeneratorPass
{
    CLASS(FlatSurfacePass, GeneratorPass);

public:
    FlatSurfacePass(const std::string& block_name);

    virtual void process(int64_t x, int64_t y, int64_t z, int64_t cx, int64_t cz, BlockState *blocks) const override;

private:
    uint16_t m_block_id;
};
