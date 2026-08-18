#include "World/Gen.hpp"

#include "World/Dimension.hpp"
#include "World/Registry.hpp"

UnderworldGen::UnderworldGen(WorldSettings settings)
    : Gen(settings)
{
}

void UnderworldGen::preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk)
{
    (void)cx;
    (void)cz;

    for (size_t i = 0; i < 16 * 16; i++)
        chunk->biomes[i] = Biome::Underworld;
}

void UnderworldGen::generate_chunk(std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim)
{
    (void)chunk;
    (void)preloaded_chunk;
    (void)dim;

    for (int64_t y = 0; y < 90; y++)
        for (int64_t x = 0; x < 16; x++)
            for (int64_t z = 0; z < 16; z++)
                chunk->get_blocks()[x + y * Chunk::width + z * Chunk::width * Chunk::height] = BlockState(Blocks::stone);
}
