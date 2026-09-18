#include "World/Gen.hpp"

#include "Core/Math.hpp"
#include "World/Biome.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <random>

void TreePass::place_small_oak_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz)
{
    int64_t x = pos.x * 16;
    int64_t z = pos.z * 16;

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_height(5, 7);
    int64_t tree_height = (int64_t)dist_tree_height(rng);

    int64_t height = tree_height + 2;
    int64_t width = 5;
    BlockState *blocks = new BlockState[width * height * width](); // FIXME: free this

    const int64_t log_xz = 2;
    for (int64_t y = 0; y < tree_height; y++)
        blocks[log_xz + y * width + log_xz * width * height] = BlockState(Blocks::oak_log);
    for (int64_t x2 = 0; x2 < 5; x2++)
        for (int64_t y2 = tree_height - 4; y2 < tree_height - 1; y2++)
            for (int64_t z2 = 0; z2 < 5; z2++)
            {
                if (!blocks[x2 + y2 * width + z2 * width * height].is_air())
                    continue;
                blocks[x2 + y2 * width + z2 * width * height] = BlockState(Blocks::oak_leaves);
            }
    for (int64_t x2 = 1; x2 < 4; x2++)
        for (int64_t y2 = tree_height - 1; y2 < tree_height; y2++)
            for (int64_t z2 = 1; z2 < 4; z2++)
            {
                if (!blocks[x2 + y2 * width + z2 * width * height].is_air())
                    continue;
                blocks[x2 + y2 * width + z2 * width * height] = BlockState(Blocks::oak_leaves);
            }
    blocks[2 + tree_height * width + 2 * width * height] = BlockState(Blocks::oak_leaves);
    blocks[2 + tree_height * width + 1 * width * height] = BlockState(Blocks::oak_leaves);
    blocks[2 + tree_height * width + 3 * width * height] = BlockState(Blocks::oak_leaves);
    blocks[1 + tree_height * width + 2 * width * height] = BlockState(Blocks::oak_leaves);
    blocks[3 + tree_height * width + 2 * width * height] = BlockState(Blocks::oak_leaves);

    int64_t elevation = chunk->heights[lx + lz * 16];
    dim.place_structure(glm::i64vec3(x + lx - width / 2, elevation, z + lz - width / 2), blocks, width, height, width);
}

void TreePass::place_big_oak_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz)
{
    int64_t x = pos.x * 16;
    int64_t z = pos.z * 16;

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_height(8, 14);
    int64_t tree_height = (int64_t)dist_tree_height(rng);

    int64_t height = tree_height + 3;
    int64_t width = 14;
    BlockState *blocks = new BlockState[width * height * width](); // FIXME: free this

    for (int64_t y = 0; y < tree_height; y++)
        for (int64_t x = 0; x < 2; x++)
            for (int64_t z = 0; z < 2; z++)
            {
                blocks[(5 + x) + y * width + (5 + z) * width * height] = BlockState(Blocks::oak_log);
            }

    for (int64_t y = 0; y < 2; y++)
        for (int64_t x = 0; x < 14; x++)
            for (int64_t z = 0; z < 14; z++)
            {
                float distance = glm::distance(glm::vec2(x, z), glm::vec2(5.5, 5.5));
                if (distance <= 6.0f)
                    blocks[x + (tree_height - 2 + y) * width + z * width * height] = BlockState(Blocks::oak_leaves);
            }

    for (int64_t x = 1; x < 13; x++)
        for (int64_t z = 1; z < 13; z++)
        {
            float distance = glm::distance(glm::vec2(x, z), glm::vec2(5.5, 5.5));
            if (distance <= 5.0f)
                blocks[x + (tree_height + 0) * width + z * width * height] = BlockState(Blocks::oak_leaves);
        }
    for (int64_t x = 1; x < 13; x++)
        for (int64_t z = 1; z < 13; z++)
        {
            float distance = glm::distance(glm::vec2(x, z), glm::vec2(5.5, 5.5));
            if (distance <= 5.0f)
                blocks[x + (tree_height - 3) * width + z * width * height] = BlockState(Blocks::oak_leaves);
        }

    int64_t elevation = chunk->heights[lx + lz * 16];
    dim.place_structure(glm::i64vec3(x + lx - width / 2, elevation, z + lz - width / 2), blocks, width, height, width);
}

void TreePass::place_spruce_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz)
{
    int64_t x = pos.x * 16;
    int64_t z = pos.z * 16;

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_height(6, 8);
    int64_t tree_height = (int64_t)dist_tree_height(rng);

    int64_t height = tree_height + 2;
    int64_t width = 7;
    BlockState *blocks = new BlockState[width * height * width](); // FIXME: free this

    const int64_t log_xz = 3;
    for (int64_t y = 0; y < tree_height; y++)
        blocks[log_xz + y * width + log_xz * width * height] = BlockState(Blocks::spruce_log);

    for (int64_t x = 0; x < 7; x++)
        for (int64_t z = 0; z < 7; z++)
        {
            float distance = glm::distance(glm::vec2(x, z), glm::vec2(3, 3));
            if (distance <= 2.0f)
                blocks[x + (tree_height - 3) * width + z * width * height] = BlockState(Blocks::oak_leaves);
        }

    for (int64_t x = 0; x < 7; x++)
        for (int64_t z = 0; z < 7; z++)
        {
            float distance = glm::distance(glm::vec2(x, z), glm::vec2(3, 3));
            if (distance <= 3.0f)
                blocks[x + (tree_height - 5) * width + z * width * height] = BlockState(Blocks::oak_leaves);
        }

    blocks[3 + (tree_height - 2) * width + 2 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[3 + (tree_height - 2) * width + 4 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[2 + (tree_height - 2) * width + 3 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[4 + (tree_height - 2) * width + 3 * width * height] = BlockState(Blocks::spruce_leaves);

    blocks[3 + tree_height * width + 3 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[3 + tree_height * width + 2 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[3 + tree_height * width + 4 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[2 + tree_height * width + 3 * width * height] = BlockState(Blocks::spruce_leaves);
    blocks[4 + tree_height * width + 3 * width * height] = BlockState(Blocks::spruce_leaves);

    int64_t elevation = chunk->heights[lx + lz * 16];
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

    std::uniform_int_distribution<std::mt19937::result_type> dist_tree_type(0, 100);
    uint64_t tree_dist = dist_tree_type(rng);

    Biome biome = chunk->biomes[lx + lz * 16];

    if (biome == Biome::Forest)
    {
        if (tree_dist > 20)
            place_small_oak_tree(pos, chunk, dim, rng, lx, lz);
        else
            place_big_oak_tree(pos, chunk, dim, rng, lx, lz);
    }
    else if (biome == Biome::ColdForest)
    {
        place_spruce_tree(pos, chunk, dim, rng, lx, lz);
    }
}

OverworldGen::OverworldGen(WorldSettings settings)
    : Gen(settings)
{
    std::vector<double> x{0.0f, 0.45f, 0.55f, 1.0f};
    std::vector<double> y{0.0f, 0.1f, 0.9f, 1.0f};
    m_continent_spline = tk::spline(x, y);

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

            float temperature = m_noise.sample(glm::vec2((float)gx, (float)gz) / 1128.0f + glm::vec2(22.01f, 123.0f)) / 2.0f + 0.5f;

            float continent_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 4000.0f) / 2.0f + 0.5f;
            float continent = (float)m_continent_spline(continent_s0);

            // float island_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 123.0f) / 2.0f + 0.5f;
            // island_s0 = island_s0 < 0.5f ? 0.0f : (island_s0 - 0.5f) / 0.5f;
            // float island = (float)m_continent_spline(island_s0);
            // float island_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 800.0f) / 2.0f + 0.5f;

            float mountain_s0_raw = m_noise.fractal<1>(glm::vec2((float)gx, (float)gz), 0.001f, 20.0, 15.0, 7.0);
            float mountain_s0 = mountain_s0_raw / 2.0f + 0.5f;
            float mountain_s1 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 80.0f) / 2.0f + 0.5f;
            float mountain_s2 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 30.0f) / 2.0f + 0.5f;
            float mountain = mountain_s0 * 80.0f + mountain_s1 * 15.0f + mountain_s2 * 3.0f;
            float mountain_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 800.0f) / 2.0f + 0.5f;

            float lakes_s0 = m_noise.sample(glm::vec2((float)gx, (float)gz) / 700.0f) / 2.0f + 0.5f;

            float forest_mask = m_noise.sample(glm::vec2((float)gx, (float)gz) / 900.0f) / 2.0f + 0.5f;

            float elevation = float(m_settings.ocean_floor);
            elevation += std::max(continent * ocean_amplitude, 5.0f);
            elevation += mountain_mask * mountain_mask * continent * mountain;
            elevation -= lakes_s0 * continent * 15.0f;
            // elevation += island * (ocean_amplitude) * (1.0f - continent) * island_mask;

            int64_t height = int64_t(elevation);
            height = std::min(height, (int64_t)255l);

            BiomeTemperature temp = BiomeTemperature::Temperate;
            if (temperature <= 0.333)
                temp = BiomeTemperature::Cold;
            else if (temperature >= 0.666)
                temp = BiomeTemperature::Hot;

            const float beach_treshold = 0.57f;

            Biome biome = Biome::Plain;
            if (temp == BiomeTemperature::Temperate)
            {
                if (mountain * mountain_mask > 52.0)
                    biome = Biome::Mountain;
                else if (height < m_settings.ocean_level && continent_s0 < beach_treshold)
                    biome = Biome::Ocean;
                else if (continent_s0 < beach_treshold)
                    biome = Biome::Beach;
                else if (forest_mask > 0.2)
                    biome = Biome::Forest;
            }
            else if (temp == BiomeTemperature::Cold)
            {
                biome = Biome::ColdPlain;
                if (mountain * mountain_mask > 52.0)
                    biome = Biome::FrozenMountain;
                else if (height < m_settings.ocean_level && continent_s0 < beach_treshold)
                    biome = Biome::FrozenOcean;
                else if (continent_s0 < beach_treshold)
                    biome = Biome::ColdPlain;
                else if (forest_mask > 0.2)
                    biome = Biome::ColdForest;
            }
            else if (temp == BiomeTemperature::Hot)
            {
                biome = Biome::Desert;
                if (mountain * mountain_mask > 52.0)
                    biome = Biome::Mountain; // TODO: do something else ?
                else if (height < m_settings.ocean_level && continent_s0 < beach_treshold)
                    biome = Biome::Ocean;
                else if (continent_s0 < beach_treshold)
                    biome = Biome::Beach;
                else if (forest_mask > 0.2)
                    biome = Biome::Desert;
            }

            chunk->heights[x + z * 16] = height;
            chunk->biomes[x + z * 16] = biome;
            chunk->mountains[x + z * 16] = mountain_s0_raw;
        }
    }
}

double get_coal_ore_density(double density, double y)
{
    return density * std::clamp((256 - y) / (256 - 80.0), 0.0, 1.0);
}

double get_diamond_ore_density(double density, double y)
{
    return density * std::clamp((256 - y) / (256 - 30.0), 0.0, 1.0);
}

void OverworldGen::generate_chunk(std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim)
{
    BlockState *blocks = chunk->get_blocks();
    ChunkPos cpos = chunk->pos();

    std::vector<StructureGen> structures;
    dim.get_structures_overlap(cpos, structures);

    BlockState stone(Blocks::stone);

    for (int64_t x = 0; x < 16; x++)
    {
        for (int64_t z = 0; z < 16; z++)
        {
            const int64_t gx = x + chunk->pos().x * 16;
            const int64_t gz = z + chunk->pos().z * 16;

            Biome biome = preloaded_chunk->biomes[x + z * 16];
            int64_t height = preloaded_chunk->heights[x + z * 16];
            float mountain = preloaded_chunk->mountains[x + z * 16];

            chunk->get_biomes()[x + z * 16] = biome;

            int64_t y = 0;
            for (; y < height - 3; y++)
                blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::stone);

            BlockState ground;
            BlockState surface;
            switch (biome)
            {
            case Biome::Forest:
            case Biome::Plain:
                ground = BlockState(Blocks::dirt);
                surface = BlockState(Blocks::grass_block);
                break;
            case Biome::ColdForest:
            case Biome::ColdPlain:
                ground = BlockState(Blocks::grass_block);
                surface = BlockState(Blocks::snow_block);
                break;
            case Biome::FrozenMountain: // TODO: do something more interesting
            case Biome::Mountain:
                ground = BlockState(Blocks::stone);
                surface = BlockState(Blocks::stone);
                break;
            case Biome::Desert:
            case Biome::Beach:
            case Biome::Ocean:
            case Biome::FrozenOcean:
                ground = BlockState(Blocks::sand);
                surface = BlockState(Blocks::sand);
                break;
            case Biome::Underworld: // unused in overworld
            case Biome::Max:
                break;
            }

            for (; y < height - 1; y++)
                blocks[x + y * 16 + z * 16 * 256] = ground;
            blocks[x + (y++) * 16 + z * 16 * 256] = surface;

            // Add snow on top of mountains
            if (height > 160 && (biome == Biome::Mountain || biome == Biome::FrozenMountain))
                blocks[x + (y - 1) * 16 + z * 16 * 256] = BlockState(Blocks::snow_block);

            // Fill oceans
            if (biome == Biome::FrozenOcean)
            {
                for (; y < m_settings.ocean_level - 1; y++)
                    chunk->set_tag({x, y, z}, "water", (int64_t)0, true);

                if (y == m_settings.ocean_level - 1)
                    blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::ice);

                float iceberg = m_noise.sample(glm::vec2((float)gx, (float)gz) / 24.0f) / 2.0f + 0.5f;

                if (std::abs(iceberg) >= 0.8f && y == m_settings.ocean_level - 1)
                {
                    int64_t iceberg_height = int64_t((iceberg - 0.8f) / 0.2f * 11.0f);

                    for (int64_t i = -iceberg_height; i < iceberg_height; i++)
                    {
                        blocks[x + (y + i) * 16 + z * 16 * 256] = BlockState(Blocks::ice);
                        chunk->remove_tag({x, y, z}, "water");
                    }
                }
            }
            else
            {
                for (; y < m_settings.ocean_level; y++)
                    chunk->set_tag({x, y, z}, "water", (int64_t)0, true);
            }

            // Caves
            for (int64_t y = 0; y < height; y++)
            {
                float top_noise = m_noise.sample(glm::vec3(gx, y, gz) * glm::vec3(0.007)) * 0.5f + 0.5f;

                float noise_a = m_noise.sample(glm::vec3(gx, y, gz) * glm::vec3(0.011));
                float noise_b = m_noise.sample(glm::vec3(gx, y, gz) * glm::vec3(0.021));
                const float threshold = 0.12;

                if (y > 3 && y < int64_t(std::round(float(height) - top_noise * 5)) && std::abs(noise_a) < threshold && std::abs(noise_b) < threshold)
                {
                    blocks[x + y * 16 + z * 16 * 256] = BlockState();
                }

                float noise_c = (m_noise.sample(glm::vec3(gx, y, gz) * glm::vec3(0.011)) * 0.5f + 0.5f);
                if (y > 2 && y < height - 7 && noise_c < 0.1)
                {
                    blocks[x + y * 16 + z * 16 * 256] = BlockState();
                }
            }

            const float river = std::abs(mountain);
            if (river < 0.04 && (biome != Biome::Ocean && biome != Biome::FrozenOcean))
            {
                int64_t river_depth = 5 - int64_t(river / 0.04 * 5.0);
                for (int64_t i = 0; i < river_depth; i++)
                {
                    blocks[x + (y - i - 1) * 16 + z * 16 * 256] = BlockState();
                    chunk->set_tag({x, y - i - 1, z}, "water", (int64_t)0, true);
                }
            }

            if ((biome == Biome::Plain || biome == Biome::Forest) && !blocks[x + (y - 1) * 16 + z * 16 * 256].is_air() && !chunk->get_tag({x, y - 1, z}, "water").has_value())
            {
                bool vegetation = (m_noise.sample(glm::vec2((float)gx, (float)gz) / 10.0f) / 2.0f + 0.5f) > 0.8f;
                if (vegetation)
                    blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::grass);
            }
            if (biome == Biome::Desert && !blocks[x + (y - 1) * 16 + z * 16 * 256].is_air() && !chunk->get_tag({x, y - 1, z}, "water").has_value())
            {
                const float cactus_treshold = 0.982f;
                float cactus_f = m_noise.sample(glm::vec2((float)gx, (float)gz)) / 2.0f + 0.5f;
                if (cactus_f >= cactus_treshold)
                {
                    // size_t size = size_t((cactus_f - cactus_treshold) / (1.0f - cactus_treshold) * 2) + 1;
                    for (size_t i = 0; i < 2; i++)
                        blocks[x + (y + i) * 16 + z * 16 * 256] = BlockState(Blocks::cactus);
                }
            }

            // Place a few layer of unbreakable "bedrock" at the bottom of the map.
            blocks[x + 0 * 16 + z * 16 * 256] = BlockState(Blocks::bedrock);
            float bedrock_noise_a = m_noise.sample(glm::vec2(x, z)) * 0.5f + 0.5f;
            float bedrock_noise_b = m_noise.sample(glm::vec2(x, z) + glm::vec2(1213.0, 23231.0)) * 0.5f + 0.5f;
            if (bedrock_noise_a >= 0.5f)
                blocks[x + 1 * 16 + z * 16 * 256] = BlockState(Blocks::bedrock);
            if (bedrock_noise_b >= 0.5f)
                blocks[x + 2 * 16 + z * 16 * 256] = BlockState(Blocks::bedrock);

            // Generate simple ores.
            for (int64_t y = 0; y < 256; y++)
            {
                float density = (float)get_coal_ore_density(m_noise.sample(glm::vec3(x, y, z) / 24.0f), (float)y);
                if (density > 0.94f && blocks[x + y * 16 + z * 16 * 256] == BlockState(Blocks::stone))
                    blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::coal_ore);
            }
            for (int64_t y = 0; y < 256; y++)
            {
                float density = (float)get_diamond_ore_density(m_noise.sample(glm::vec3(x, y, z) / 12.0f), (float)y);
                if (density > 0.9f && blocks[x + y * 16 + z * 16 * 256] == BlockState(Blocks::stone))
                    blocks[x + y * 16 + z * 16 * 256] = BlockState(Blocks::diamond_ore);
            }
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
