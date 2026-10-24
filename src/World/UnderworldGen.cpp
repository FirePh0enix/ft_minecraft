#include "World/Gen.hpp"

#include "World/Dimension.hpp"
#include "World/Registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>

UnderworldGen::UnderworldGen(WorldSettings settings)
    : Gen(settings)
{
}

glm::vec2 UnderworldGen::cavern_bounds(int64_t x, int64_t z) const
{
    const glm::vec2 pos{float(x), float(z)};
    const float hills = m_noise.fractal<3>(pos + glm::vec2(913, -527), 0.008f, 1.0f, 2.0f, 0.5f);
    const float detail = m_noise.sample(pos / 31.0f);
    const float ceiling = m_noise.fractal<3>(pos + glm::vec2(-617, 1381), 0.011f, 1.0f, 2.0f, 0.5f);
    return {std::clamp(43.0f + hills * 24.0f + detail * 5.0f, 20.0f, 66.0f),
            std::clamp(103.0f + ceiling * 18.0f, 83.0f, 117.0f)};
}

void UnderworldGen::preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk)
{
    for (int64_t z = 0; z < Chunk::width; ++z)
        for (int64_t x = 0; x < Chunk::width; ++x)
        {
            const auto bounds = cavern_bounds(cx * Chunk::width + x, cz * Chunk::width + z);
            const size_t index = x + z * Chunk::width;
            chunk->biomes[index] = Biome::Underworld;
            chunk->heights[index] = int64_t(bounds.x);
            chunk->mountains[index] = bounds.y;
        }
}

void UnderworldGen::generate_chunk(std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim)
{
    (void)dim;
    std::copy(preloaded_chunk->biomes.begin(), preloaded_chunk->biomes.end(), chunk->get_biomes().begin());
    std::fill_n(chunk->get_blocks(), Chunk::block_count, BlockState());

    // Sample the existing 3D noise on a coarse lattice, then interpolate per voxel.
    // Shared world-space samples make adjacent chunks agree at their boundaries.
    constexpr int spacing = 4;
    constexpr int horizontal_samples = Chunk::width / spacing + 1;
    constexpr int vertical_samples = (roof_height + 1) / spacing + 1;
    std::array<float, horizontal_samples * horizontal_samples * vertical_samples> density{};
    const auto index = [](int x, int y, int z)
    {
        return x + horizontal_samples * (z + horizontal_samples * y);
    };
    for (int z = 0; z < horizontal_samples; ++z)
        for (int x = 0; x < horizontal_samples; ++x)
        {
            const int64_t gx = chunk->x() * Chunk::width + x * spacing;
            const int64_t gz = chunk->z() * Chunk::width + z * spacing;
            const auto bounds = cavern_bounds(gx, gz);
            const float pillars = m_noise.sample((glm::vec2(float(gx), float(gz)) + glm::vec2(281, -419)) / 55.0f);
            for (int y = 0; y < vertical_samples; ++y)
            {
                const float gy = float(y * spacing);
                const float shape = m_noise.fractal<3>(glm::vec3(float(gx) + 367, gy * 1.3f, float(gz) - 811),
                                                       0.026f, 1.0f, 2.0f, 0.5f);
                const float boundary = std::max((bounds.x - gy) / 18.0f, (gy - bounds.y) / 16.0f);
                const float column = (pillars - 0.38f) * 3.5f + shape * 0.65f;
                density[index(x, y, z)] = std::max(boundary + shape * 1.6f, column);
            }
        }

    for (int z = 0; z < Chunk::width; ++z)
        for (int x = 0; x < Chunk::width; ++x)
        {
            const int64_t gx = chunk->x() * Chunk::width + x;
            const int64_t gz = chunk->z() * Chunk::width + z;
            const float bedrock_noise = m_noise.sample(glm::vec2(float(gx) + 37, float(gz) - 71) / 3.0f);
            const int bedrock_depth = 1 + int(std::clamp((bedrock_noise + 1.0f) * 1.5f, 0.0f, 3.0f));
            const float soil = m_noise.sample(glm::vec2(float(gx) - 147, float(gz) + 293) / 23.0f);
            const int ix = x / spacing, iz = z / spacing;
            const float tx = float(x % spacing) / spacing, tz = float(z % spacing) / spacing;
            for (int y = 0; y <= roof_height; ++y)
            {
                BlockState block;
                if (y < bedrock_depth || y > roof_height - bedrock_depth)
                    block = BlockState(Blocks::bedrock);
                else
                {
                    const int iy = y / spacing;
                    const float ty = float(y % spacing) / spacing;
                    const float low = std::lerp(std::lerp(density[index(ix, iy, iz)], density[index(ix + 1, iy, iz)], tx),
                                                std::lerp(density[index(ix, iy, iz + 1)], density[index(ix + 1, iy, iz + 1)], tx), tz);
                    const float high = std::lerp(std::lerp(density[index(ix, iy + 1, iz)], density[index(ix + 1, iy + 1, iz)], tx),
                                                 std::lerp(density[index(ix, iy + 1, iz + 1)], density[index(ix + 1, iy + 1, iz + 1)], tx), tz);
                    if (std::lerp(low, high, ty) > 0.0f)
                        block = BlockState(soil > 0.3f && y < preloaded_chunk->heights[x + z * Chunk::width] + 3
                                               ? Blocks::dirt
                                               : Blocks::stone);
                }
                chunk->get_blocks()[Chunk::linearize(x, y, z)] = block;
            }
        }
}
