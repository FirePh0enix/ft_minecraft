#include "World/Biome.hpp"
#include "World/Gen.hpp"

#include "World/Registry.hpp"

OverworldGen::OverworldGen(WorldSettings settings)
    : Gen(settings)
{
    std::vector<double> x{0.0f, 0.45f, 0.55f, 1.0f};
    std::vector<double> y{0.0f, 0.1f, 0.9f, 1.0f};
    m_continent_spline = tk::spline(x, y);
}

void OverworldGen::generate_chunk(std::shared_ptr<Chunk> chunk)
{
    BlockState *blocks = chunk->get_blocks();
    Biome *biomes = chunk->get_biomes();
    int64_t cx = chunk->x();
    int64_t cz = chunk->z();

    float ocean_amplitude = float(m_settings.ocean_level) - float(m_settings.ocean_floor) + 3.0f;

    for (int64_t x = 0; x < 16; x++)
    {
        for (int64_t z = 0; z < 16; z++)
        {
            int64_t gx = x + cx * 16;
            int64_t gz = z + cz * 16;

            float continent_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 4000.0f) / 2.0f + 0.5f;
            float continent = (float)m_continent_spline(continent_s0);

            float mountain_s0 = m_noise.fractal<1>(glm::vec2((float)gx, (float)gz), 0.001f, 20.0, 15.0, 7.0) / 2.0f + 0.5f;
            float mountain_s1 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 80.0f) / 2.0f + 0.5f;
            float mountain_s2 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 30.0f) / 2.0f + 0.5f;
            float mountain = mountain_s0 * 100.0f + mountain_s1 * 20.0f + mountain_s2 * 5.0f;

            float lakes_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 700.0f) / 2.0f + 0.5f;

            float mountain_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 800.0f) / 2.0f + 0.5f;

            Biome biome = Biome::Plain;
            if (mountain * mountain_mask > 52.0)
                biome = Biome::Mountain;
            else if (continent_s0 < 0.62f)
                biome = Biome::Beach;

            float elevation = float(m_settings.ocean_floor);
            elevation += continent * (ocean_amplitude);
            elevation += mountain_mask * mountain_mask * continent * mountain;
            elevation -= lakes_s0 * 15.0f;

            int64_t height = int64_t(elevation);
            height = std::min(height, 255l);

            int64_t y = 0;
            for (; y < height - 3; y++)
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::stone);

            biomes[x + z * 16] = biome;

            BlockState ground;
            BlockState surface;
            switch (biome)
            {
            case Biome::Forest:
            case Biome::Plain:
                ground = BlockState(Blocks::dirt);
                surface = BlockState(Blocks::grass);
                break;
            case Biome::Mountain:
                ground = BlockState(Blocks::stone);
                surface = BlockState(Blocks::stone);
                break;
            case Biome::Desert:
            case Biome::Beach:
            case Biome::Ocean:
                ground = BlockState(Blocks::sand);
                surface = BlockState(Blocks::sand);
                break;
            }

            for (; y < height - 1; y++)
                blocks[x + y * 16 + z * 16 * 256] = ground;
            blocks[x + (y++) * 16 + z * 16 * 256] = surface;

            // Add snow on top of mountains
            if (mountain * mountain_mask > 90.0)
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::snow);

            // Fill oceans
            for (; y < m_settings.ocean_level; y++)
                chunk->set_tag({x, y, z}, "water", int64_t(0));

            y = 0;
            for (; y < height; y++)
            {
                float caves_s0 = m_noise.sample(glm::vec3((float)gx, (float)y, (float)gz) / 100.0f);
                float caves = caves_s0 * (1.0f - (float)y / ((float)height + 2));
                if (caves > 0.5f)
                    blocks[x + y * 16 + z * 16 * 256] = BlockState();
            }
        }
    }
}
