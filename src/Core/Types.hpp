#pragma once

#include <cstdint>

struct Extent2D
{
    uint32_t width;
    uint32_t height;

    Extent2D()
        : width(0), height(0)
    {
    }

    Extent2D(uint32_t width, uint32_t height)
        : width(width), height(height)
    {
    }
};

/**
 * Structures that are used to share between the CPU and GPU should use this attribute, or manually pad the structure
 * to respect GPU requirements.
 */
#define GPU_ATTRIBUTE __attribute__((aligned(16)))
