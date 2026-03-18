#include "World/Pass/FlatSurface.hpp"
#include "World/Registry.hpp"

FlatSurfacePass::FlatSurfacePass(const std::string& block_name)
{
    m_block_id = BlockRegistry::get_block_id(block_name);
}

void FlatSurfacePass::process(int64_t x, int64_t chunk_start_y, int64_t z, int64_t local_x, int64_t local_z, BlockState *blocks) const
{
    (void)x;
    (void)z;

    for (int64_t y = 0; y < Chunk::width + 2; y++)
    {
        int64_t global_y = chunk_start_y + y - 1;
        if (global_y < 10)
            blocks[Chunk::linearize_with_overlap(local_x, y, local_z)] = BlockState(m_block_id);
    }
}
