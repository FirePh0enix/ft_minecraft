#include "World/Gen.hpp"

void MountainPass::init(GenDesc& desc)
{
    m_flat = true;
    desc.add_buffer("mountain", sizeof(float));
}

void MountainPass::gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    (void)y;
    (void)state;
    (void)tags;
    (void)biome;
    float m = m_noise.sample((glm::vec2(x, z) + glm::vec2(0.1, -0.3)) / 100.0f) / 2.0f + 0.5f;

    float *mountain = gen.get_buffer<float>("mountain");
    *mountain = m * m;
}
