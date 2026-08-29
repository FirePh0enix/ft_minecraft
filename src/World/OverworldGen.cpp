#include "World/Gen.hpp"

#include "Core/Math.hpp"
#include "Engine.hpp"
#include "World/Biome.hpp"
#include "World/Registry.hpp"

#include <random>

#define TREE_TYPE_SHORT 0
#define TREE_TYPE_BIG 1

void TreePass::place_short_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz)
{
    int64_t x = pos.x * 16;
    int64_t z = pos.z * 16;

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_height(5, 7);
    int64_t tree_height = (int64_t)dist_tree_height(rng);

    int64_t height = tree_height + 3;
    int64_t width = 9;

    if (chunk->biomes[lx + lz * 16] != Biome::Plain)
        return;

    int64_t elevation = chunk->heights[lx + lz * 16];
    const int64_t log_xz = width / 2 + 1;

    BlockState *blocks = new BlockState[width * height * width](); // TODO: free this
    for (int64_t y = 0; y < tree_height; y++)
        blocks[log_xz + y * width + log_xz * width * height] = Engine::get().registry().get_default_state(Blocks::log);

    const int64_t core_x = log_xz;
    const int64_t core_y = tree_height - 2;
    const int64_t core_z = log_xz;
    for (int64_t leave_x = -3; leave_x <= 3; leave_x++)
        for (int64_t leave_z = -3; leave_z <= 3; leave_z++)
            for (int64_t leave_y = -2; leave_y <= 2; leave_y++)
            {
                float distance = glm::distance2(glm::vec3(core_x, core_y, core_z), glm::vec3(core_x + leave_x, core_y + leave_y, core_z + leave_z));
                const int64_t index = (core_x + leave_x) + (core_y + leave_y) * width + (core_z + leave_z) * width * height;
                if (blocks[index].is_air() && distance < 3 * 3)
                    blocks[index] = Engine::get().registry().get_default_state(Blocks::leaves);
            }

    dim.place_structure(glm::i64vec3(x + lx - width / 2, elevation, z + lz - width / 2), blocks, width, height, width);
}

void TreePass::place_big_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz)
{
    int64_t x = pos.x * 16;
    int64_t z = pos.z * 16;

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_height(8, 14);
    int64_t tree_height = (int64_t)dist_tree_height(rng);

    int64_t height = tree_height + 3;
    int64_t width = 13;

    if (chunk->biomes[lx + lz * 16] != Biome::Plain)
        return;

    int64_t elevation = chunk->heights[lx + lz * 16];
    const int64_t log_xz = width / 2 + 1;

    BlockState *blocks = new BlockState[width * height * width](); // TODO: free this
    for (int64_t y = 0; y < tree_height; y++)
        blocks[log_xz + y * width + log_xz * width * height] = Engine::get().registry().get_default_state(Blocks::log);

    const int64_t core_x = log_xz;
    const int64_t core_y = tree_height - 3;
    const int64_t core_z = log_xz;
    for (int64_t leave_x = -5; leave_x <= 5; leave_x++)
        for (int64_t leave_z = -5; leave_z <= 5; leave_z++)
            for (int64_t leave_y = -4; leave_y <= 4; leave_y++)
            {
                float distance = glm::distance2(glm::vec3(core_x, core_y, core_z), glm::vec3(core_x + leave_x, core_y + leave_y, core_z + leave_z));
                const int64_t index = (core_x + leave_x) + (core_y + leave_y) * width + (core_z + leave_z) * width * height;
                if (blocks[index].is_air() && distance < 5 * 5)
                    blocks[index] = Engine::get().registry().get_default_state(Blocks::leaves);
            }

    dim.place_structure(glm::i64vec3(x + lx - width / 2, elevation, z + lz - width / 2), blocks, width, height, width);
}

void TreePass::place(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    rng.seed((pos.x * 73856093) ^ (pos.z * 19349663));
    std::uniform_int_distribution<std::mt19937::result_type> dist016(0, 15);
    int64_t lx = (int64_t)dist016(rng);
    int64_t lz = (int64_t)dist016(rng);

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_type(0, 1);
    uint64_t tree_type = dist_tree_type(rng);

    if (tree_type == TREE_TYPE_SHORT)
        place_short_tree(pos, chunk, dim, rng, lx, lz);
    else if (tree_type == TREE_TYPE_BIG)
        place_big_tree(pos, chunk, dim, rng, lx, lz);
}

OverworldGen::OverworldGen(WorldSettings settings)
    : Gen(settings)
{
    std::vector<double> x{0.0f, 0.45f, 0.55f, 1.0f};
    std::vector<double> y{0.0f, 0.1f, 0.9f, 1.0f};
    m_continent_spline = tk::spline(x, y);

    m_tree = Engine::get().registry().get_struct("tree");

    m_structure_passes.push_back(std::make_shared<TreePass>());
}

void OverworldGen::preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk)
{
    const float ocean_amplitude = float(m_settings.ocean_level) - float(m_settings.ocean_floor) + 3.0f;

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

            float forest_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 900.0f) / 2.0f + 0.5f;

            Biome biome = Biome::Plain;
            if (mountain * mountain_mask > 52.0)
                biome = Biome::Mountain;
            else if (continent_s0 < 0.62f)
                biome = Biome::Beach;
            else if (forest_mask > 0.2)
                biome = Biome::Forest;

            float elevation = float(m_settings.ocean_floor);
            elevation += continent * (ocean_amplitude);
            elevation += mountain_mask * mountain_mask * continent * mountain;
            elevation -= lakes_s0 * 15.0f;

            int64_t height = int64_t(elevation);
            height = std::min(height, (int64_t)255l);

            chunk->heights[x + z * 16] = height;
            chunk->biomes[x + z * 16] = biome;
        }
    }
}

void OverworldGen::generate_chunk(std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim)
{
    BlockState *blocks = chunk->get_blocks();
    ChunkPos cpos = chunk->pos();

    std::vector<StructureGen> structures;
    dim.get_structures_overlap(cpos, structures);

    const BlockState stone = Engine::get().registry().get_default_state(Blocks::stone);
    const BlockState dirt = Engine::get().registry().get_default_state(Blocks::dirt);
    const BlockState grass = Engine::get().registry().get_default_state(Blocks::grass);
    const BlockState sand = Engine::get().registry().get_default_state(Blocks::sand);
    const BlockState snow = Engine::get().registry().get_default_state(Blocks::snow);

    for (int64_t x = 0; x < 16; x++)
    {
        for (int64_t z = 0; z < 16; z++)
        {
            // if (st.stop_requested())
            //     return;

            Biome biome = preloaded_chunk->biomes[x + z * 16];
            int64_t height = preloaded_chunk->heights[x + z * 16];

            int64_t y = 0;
            for (; y < height - 3; y++)
                blocks[x + y * 16 + z * 16 * 256] = stone;

            BlockState ground;
            BlockState surface;
            switch (biome)
            {
            case Biome::Forest:
            case Biome::Plain:
                ground = dirt;
                surface = grass;
                break;
            case Biome::Mountain:
                ground = stone;
                surface = stone;
                break;
            case Biome::Desert:
            case Biome::Beach:
            case Biome::Ocean:
                ground = sand;
                surface = sand;
                break;
            case Biome::Underworld:
                break;
            }

            for (; y < height - 1; y++)
                blocks[x + y * 16 + z * 16 * 256] = ground;
            blocks[x + (y++) * 16 + z * 16 * 256] = surface;

            // Add snow on top of mountains
            if (height > 160 && biome == Biome::Mountain)
                blocks[x + y * 16 + z * 16 * 256] = snow;

            // Fill oceans
            for (; y < m_settings.ocean_level; y++)
                chunk->set_tag({x, y, z}, "water", (int64_t)0, true);
        }
    }

    for (const StructureGen& gen : structures)
    {
        for (int64_t sx = 0; sx < gen.w; sx++)
        {
            if (chunk_index(gen.pos.x + sx) != cpos.x)
                continue;

            int64_t lx = local_coords(gen.pos.x + sx);
            for (int64_t sz = 0; sz < gen.l; sz++)
            {
                if (chunk_index(gen.pos.z + sz) != cpos.z)
                    continue;

                int64_t lz = local_coords(gen.pos.z + sz);
                for (int64_t sy = 0; sy < gen.h; sy++)
                {
                    if (gen.pos.y + sy < 0 || gen.pos.y + sy > 255)
                        continue;

                    glm::i64vec3 pos = glm::i64vec3(lx, gen.pos.y + sy, lz);

                    BlockState block = gen.blocks[sx + sy * gen.w + sz * gen.w * gen.h];
                    if (!block.is_air())
                        blocks[pos.x + pos.y * Chunk::width + pos.z * Chunk::width * Chunk::height] = block;
                }
            }
        }
    }
}
