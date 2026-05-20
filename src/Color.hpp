#pragma once

#include <cstdint>

struct Color
{
    float r;
    float g;
    float b;
    float a;

    constexpr Color()
        : Color(0.0, 0.0, 0.0)
    {
    }

    constexpr Color(float r, float g, float b, float a = 1.0)
        : r(r), g(g), b(b), a(a)
    {
    }

    constexpr Color(float v, float a = 1.0)
        : r(v), g(v), b(v), a(a)
    {
    }

    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF)
    {
        return Color(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
    }

    constexpr Color alpha(float a) const
    {
        return Color(r, g, b, a);
    }
};

namespace Colors
{
static constexpr Color black = Color(0.0);
static constexpr Color white = Color(1.0);

static constexpr Color red = Color(1.0, 0.0, 0.0);
static constexpr Color green = Color(0.0, 1.0, 0.0);
static constexpr Color blue = Color(0.0, 0.0, 1.0);

static constexpr Color magenta = Color(1.0, 0.0, 1.0);
static constexpr Color yellow = Color(1.0, 1.0, 0.0);
static constexpr Color cyan = Color(0.0, 1.0, 1.0);
} // namespace Colors
