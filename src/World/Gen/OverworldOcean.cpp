#include "World/Gen.hpp"

void OverworldOceanPass::init(GenDesc& desc)
{
    m_flat = true;
    desc.add_buffer("ocean", sizeof(float));
}

void OverworldOceanPass::gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    (void)y;
    (void)state;
    (void)tags;
    (void)biome;

    const float o1 = m_noise.sample(glm::vec2(x, z) / 600.0f) / 2.0f + 0.5f;
    // const float o2 = m_noise.sample(glm::vec2(x, z) / 50.0f) / 2.0f + 0.5f;

    float *ocean = gen.get_buffer<float>("ocean");
    *ocean = o1;
}
