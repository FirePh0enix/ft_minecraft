#include "World/Pass/Overworld.hpp"

Biome OverworldBiomePass::generate_biome(int64_t x, int64_t y, int64_t z)
{
    (void)x;
    (void)y;
    (void)z;
    return Biome::Plain;
}

BlockState OverworldSurfacePass::generate_block(int64_t x, int64_t y, int64_t z, Biome biome)
{
    (void)biome;

    static constexpr int64_t base_height = 60;
    static constexpr int64_t amplitude = 80;

    // float noise_sample = (m_noise.sample(glm::vec2(x, z) * 0.1f) + 1.0f) / 2.0f;
    // int64_t height = int64_t(noise_sample * amplitude + base_height);

    int64_t height = int64_t(std::sin(float(x) / 2.0f)) + base_height;

    if (y <= height)
    {
        return BlockState(2);
    }

    // if (y <= 10)
    //     return BlockState(2);

    return BlockState();
}
