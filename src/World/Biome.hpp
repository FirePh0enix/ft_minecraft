#pragma once

#include <cstdint>

enum class BiomeTemperature : uint8_t
{
    Cold,
    Temperate,
    Hot,
};

enum class Biome : uint16_t
{
    Plain,
    ColdPlain,
    Forest,
    ColdForest,
    Desert,
    Beach,
    Mountain,
    FrozenMountain,
    Ocean,
    FrozenOcean,

    Underworld,

    Max,
};

extern const char *biome_names[];
