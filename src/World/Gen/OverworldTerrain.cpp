#include "World/Gen.hpp"

#include "World/World.hpp"
#include "World/Registry.hpp"

void OverworldTerrainPass::gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags)
{
    float ocean = *gen.get_buffer<float>("ocean"); // [0,1] = 1 is ocean, 0 is land
    float mountain = *gen.get_buffer<float>("mountain");

    float river = m_noise.sample(glm::vec2(x, z) / 500.0f) / 2.0 + 0.5f;
    if (river > 0.4) {
	river = 1;
    } else {
	river = 0;
    }

    constexpr float ocean_floor = 10;
    constexpr float ocean_amplitude = 50;

    float c = ocean * ocean;
    float m = mountain * (ocean * ocean);

    int64_t height = ocean_floor;
    height += c * ocean_amplitude;
    height += m * 30.0;
    height -= river;

    if (y == height && river > 0) {
	tags.tags.put("water", int64_t(1));
    } else if (y <= height) {
	state = BlockState(Blocks::stone);
    } else if (y <= 40) {
	tags.tags.put("water", int64_t(1));
    }
}
