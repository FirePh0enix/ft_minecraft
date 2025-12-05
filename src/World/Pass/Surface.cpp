#include "World/Pass/Surface.hpp"

BlockState SurfacePass::process(GeneratorPass *previous_pass, int64_t x, int64_t y, int64_t z) const
{
    if (y < 10)
        return BlockState(1);
    return BlockState();
}
