#include "World/Gen.hpp"

void HeightPass::init(GenDesc& desc)
{
    m_flat = true;
    desc.add_buffer("height", sizeof(float));

    (void)desc;

    std::vector<double> xa{0.0f, 0.4f, 0.6f, 1.0f};
    std::vector<double> ya{0.0f, 5.0f, 35.0f, 40.0f};
    m_spline = tk::spline(xa, ya);

    std::vector<double> xa2{0.0f, 0.3f, 1.0f};
    std::vector<double> ya2{0.0f, 38.0f, 53.0f};
    m_mspline = tk::spline(xa2, ya2);

    std::vector<double> xa3{0.0f, 0.4f, 0.6f, 1.0f};
    std::vector<double> ya3{2.0f, 2.0f, 12.0f, 12.0f};
    m_rspline = tk::spline(xa3, ya3);
}

void HeightPass::gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome)
{
    (void)y;
    (void)state;
    (void)tags;
    (void)biome;

    float ocean = *gen.get_buffer<float>("ocean");
    float mountain = *gen.get_buffer<float>("mountain");

    double river = std::abs(m_noise.sample(glm::vec2(x, z) / 230.0f));
    constexpr float river_threshold = 0.3;
    if (river <= river_threshold)
        river = 1.0 - river / river_threshold;
    else
        river = 0;

    double height = (double)gen.settings().ocean_floor;
    height += m_spline(ocean);
    height += m_mspline(mountain * (1 - river) * ocean);
    // height -= m_rspline(river);
    height -= river * 13;

    *gen.get_buffer<float>("height") = (float)height;
}
