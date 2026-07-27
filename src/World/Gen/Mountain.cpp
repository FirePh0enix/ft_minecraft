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

    const float plain_mask = m_noise.sample((glm::vec2(x, z) + 0.5f) / 400.0f);

    float *mountain = gen.get_buffer<float>("mountain");
    *mountain = m * m * (1.0f - plain_mask) * (1.0f - plain_mask);
}
