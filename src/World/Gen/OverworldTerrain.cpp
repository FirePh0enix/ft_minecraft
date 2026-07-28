#include "World/Gen.hpp"

#include "World/Registry.hpp"

void OverworldTerrainPass::init(GenDesc& desc)
{
    (void)desc;
}

void OverworldTerrainPass::gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    (void)x;
    (void)z;
    float h = *gen.get_buffer<float>("height");

    constexpr int64_t ocean_level = 48;

    int64_t height = std::max(std::min(int64_t(h), 255l), 0l);
    if (y >= height - 4 && y <= height)
    {
        // Upper layer of blocks.
        switch (biome)
        {
        case Biome::Ocean:
            state = BlockState(Blocks::sand);
            break;
        case Biome::Mountain:
            state = BlockState(Blocks::stone);
            break;
        case Biome::Forest:
        case Biome::Plain:
            state = BlockState(Blocks::dirt);
            break;
        case Biome::Beach:
        case Biome::Desert:
            state = BlockState(Blocks::sand);
            break;
        }
    }
    else if (y <= height)
    {
        state = BlockState(Blocks::stone);
    }
    else if (y <= ocean_level)
    {
        tags.tags["water"] = int64_t(1);
    }
}
