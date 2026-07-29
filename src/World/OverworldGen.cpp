#include "World/Gen.hpp"

#include "World/Registry.hpp"

void OverworldGen::generate_chunk(std::shared_ptr<Chunk> chunk)
{
    BlockState *blocks = chunk->get_blocks();
    int64_t cx = chunk->x();
    int64_t cz = chunk->z();

    for (int64_t x = 0; x < 16; x++)
    {
        for (int64_t z = 0; z < 16; z++)
        {
            int64_t gx = x + cx * 16;
            int64_t gz = z + cz * 16;

            float ocean_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 1200.0f);

            float lakes = glm::max(m_noise.sample(glm::vec2((float)gx, (float)gz) / 500.0f) / 2.0f + 0.5f - 0.3f, 0.0f) / 0.7f * 14.0f;

            float mountain_s0 = m_noise.fractal<1>(glm::vec2((float)gx, (float)gz), 0.001f, 20.0, 15.0, 7.0) / 2.0f + 0.5f;
            float mountain_s1 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 80.0f) / 2.0f + 0.5f;
            float mountain_s2 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 30.0f) / 2.0f + 0.5f;
            float mountain = mountain_s0 * 120.0f + mountain_s1 * 20.0f + mountain_s2 * 5.0f;

            float mountain_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 800.0f) / 2.0f + 0.5f;

            int64_t height = 30 + int64_t(ocean_s0 * 20.0 + mountain_mask * mountain - lakes);

            int64_t y = 0;
            for (; y < height; y++)
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::stone);

            if (mountain > 90.0)
            {
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::snow);
            }
            else if (mountain > 82.0)
            {
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::stone);
            }
            else if (ocean_s0 < -0.2f && mountain < 50)
            {
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::sand);
            }
            else
            {
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::dirt);
            }

            // Fill oceans
            for (; y < 30; y++)
                chunk->set_tag({x, y, z}, "water", int64_t(0));

            // Fill lakes
            for (; y < height - int64_t(lakes); y++)
                chunk->set_tag({x, y, z}, "water", int64_t(0));
        }
    }
}
