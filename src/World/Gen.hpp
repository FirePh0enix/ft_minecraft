#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Chunk.hpp"
#include "World/Settings.hpp"
#include "World/Structure.hpp"
#include "spline.hpp"

#include <memory>
#include <random>

struct StructureGen
{
    glm::i64vec3 pos;
    int64_t w;
    int64_t h;
    int64_t l;
    BlockState *blocks;

    StructureGen(glm::i64vec3 pos, BlockState *blocks, int64_t w, int64_t h, int64_t l)
        : pos(pos), w(w), h(h), l(l), blocks(blocks)
    {
    }
};

class GenPass;
class StructurePass;

struct PreLoadedChunk;

class StructurePass
{
public:
    virtual void place(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim) = 0;
};

class TreePass : public StructurePass
{
public:
    void place_short_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz);
    void place_big_tree(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim, std::mt19937& rng, int64_t lx, int64_t lz);

    virtual void place(ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim) override;
};

class Gen
{
public:
    Gen(WorldSettings settings)
        : m_settings(settings), m_noise(settings.seed)
    {
    }

    void add_structure_pass(std::shared_ptr<StructurePass> pass) { m_structure_passes.push_back(pass); }

    virtual void preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk) = 0;
    virtual void generate_chunk(Chunk *chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim) = 0;

    void structure_pass(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk, Dimension& dim)
    {
        for (std::shared_ptr<StructurePass> pass : m_structure_passes)
            pass->place(ChunkPos(cx, cz), chunk, dim);
    }

protected:
    WorldSettings m_settings;
    SimplexNoise m_noise;

    std::vector<std::shared_ptr<StructurePass>> m_structure_passes;
};

class OverworldGen : public Gen
{
public:
    OverworldGen(WorldSettings settings);

    virtual void preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk) override;
    virtual void generate_chunk(Chunk *chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim) override;

private:
    tk::spline m_continent_spline;
    std::shared_ptr<Structure> m_tree;
};

class UnderworldGen : public Gen
{
public:
    UnderworldGen(WorldSettings settings);

    virtual void preload(int64_t cx, int64_t cz, std::shared_ptr<PreLoadedChunk> chunk) override;
    virtual void generate_chunk(Chunk *chunk, std::shared_ptr<PreLoadedChunk> preloaded_chunk, Dimension& dim) override;

private:
};
