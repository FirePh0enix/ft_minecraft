#pragma once

#include "Core/Math.hpp"

#include <cstdint>

class SimplexNoise
{
public:
    SimplexNoise(uint64_t seed);

    float sample(glm::vec2 coords) const;
    float sample(glm::vec3 coords) const;

    template <const size_t octaves>
    float fractal(glm::vec3 coords, float frequency, float amplitude, float lacunarity, float persistence) const
    {
        float sum = 0.0;
        float norm = 0.0;

        glm::vec3 pos = coords;

        for (size_t i = 0; i < octaves; i++)
        {
            sum += amplitude * sample(pos * frequency);
            norm += amplitude;

            frequency *= lacunarity;
            amplitude *= persistence;
        }

        return sum / norm;
    }

private:
    uint8_t m_perms[256];

    inline float gradient(int32_t hash, float x) const
    {
        int h = hash & 0x0F;
        float grad = 1.0f + float(h & 7);
        if ((h & 8) != 0)
            grad = -grad;
        return grad * x;
    }

    inline float gradient(int hash, float x, float y) const
    {
        int h = hash & 0x3F;
        float u = (h < 4) ? x : y;
        float v = (h < 4) ? y : x;
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0f * v : 2.0f * v);
    }

    inline float gradient(int hash, float x, float y, float z) const
    {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -v : v);
    }
};
