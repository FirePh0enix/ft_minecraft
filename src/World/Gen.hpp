#pragma once

#include "Core/Noise/Simplex.hpp"
#include "World/Chunk.hpp"
#include "World/Settings.hpp"

#include <memory>

class GenPass;
class StructurePass;

class Gen
{
public:
    Gen(WorldSettings settings)
        : m_settings(settings), m_noise(settings.seed)
    {
    }

    virtual void generate_chunk(std::shared_ptr<Chunk> chunk) = 0;

protected:
    WorldSettings m_settings;
    SimplexNoise m_noise;
};

class OverworldGen : public Gen
{
public:
    OverworldGen(WorldSettings settings)
        : Gen(settings)
    {
    }

    virtual void generate_chunk(std::shared_ptr<Chunk> chunk) override;

private:
};
