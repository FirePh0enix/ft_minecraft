#include "World/Gen.hpp"

#include "World/World.hpp"

void OverworldOceanPass::init(GenDesc& desc)
{
    m_flat = true;
    desc.add_buffer("ocean", sizeof(float));
}

void OverworldOceanPass::gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags)
{
    int64_t lx = local_coords(x);
    int64_t lz = local_coords(z);
    
    float *ocean = gen.get_buffer<float>("ocean");

    uint16_t flat_index = lx + lz * 16;
    *ocean = m_noise.fractal<1>(glm::vec2(x, z) / 300.0f, 0.5, 0.3, 0.7, 0.2) / 2.0f + 0.5f;
}
