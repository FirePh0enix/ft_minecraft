#include "World/Gen.hpp"

#include "World/Registry.hpp"

void OverworldTerrainPass::init(GenDesc& desc)
{
    (void)desc;

    std::vector<double> xa{0.0f, 0.4f, 0.6f, 1.0f};
    std::vector<double> ya{0.0f, 5.0f, 35.0f, 40.0f};
    m_spline = tk::spline(xa, ya);

    std::vector<double> xa2{0.0f, 0.3f, 1.0f};
    std::vector<double> ya2{0.0f, 38.0f, 53.0f};
    m_mspline = tk::spline(xa2, ya2);
}

void OverworldTerrainPass::gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    float ocean = *gen.get_buffer<float>("ocean");
    float mountain = *gen.get_buffer<float>("mountain");

    double river = std::abs(m_noise.sample(glm::vec2(x, z) / 230.0f));
    constexpr float river_threshold = 0.3;
    if (river <= river_threshold)
        river = 1.0 - river / river_threshold;
    else
        river = 0;

    constexpr float ocean_floor = 10;
    constexpr int64_t ocean_level = 48;

    const float plain_mask = m_noise.sample((glm::vec2(x, z) + 0.5f) / 400.0f);

    int64_t height = ocean_floor;
    height += int64_t(m_spline(ocean));
    height += int64_t(m_mspline(mountain * (1 - river) * ocean * (1.0 - plain_mask) * (1.0 - plain_mask)));
    height -= int64_t(river * 14);

    height = std::max(std::min(height, 255l), 0l);
    if (y >= height - 4 && y <= height)
    {
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
        tags.tags.put("water", int64_t(1));
    }
}
