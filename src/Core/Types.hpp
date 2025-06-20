#pragma once

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
