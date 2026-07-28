#include "World/Gen.hpp"

#include "World/Biome.hpp"

void OverworldBiomePass::init(GenDesc& desc)
{
    (void)desc;
    m_flat = true;
}

void OverworldBiomePass::gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    (void)x;
    (void)y;
    (void)z;
    (void)state;
    (void)tags;

    const float ocean = *gen.get_buffer<float>("ocean");
    const float mountain = *gen.get_buffer<float>("mountain");

    if (ocean < 0.5)
    {
        biome = Biome::Ocean;
    }
    else if (mountain > 0.1)
    {
        biome = Biome::Mountain;
    }
    else if (ocean >= 0.5 && ocean <= 0.72)
    {
        biome = Biome::Beach;
    }
}
