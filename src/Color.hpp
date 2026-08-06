#pragma once

#include "Core/Math.hpp"

#include <cstdint>

struct __attribute__((aligned(4))) ColorInt
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

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

    /// Returns a color with the same RGB and the specified alpha value.
    constexpr Color alpha(float a) const
    {
        return Color(r, g, b, a);
    }

    constexpr ColorInt to_int() const
    {
        return ColorInt(uint8_t(r * 255.0), uint8_t(g * 255.0), uint8_t(b * 255.0), uint8_t(a * 255.0));
    }

    constexpr glm::vec4 to_vec() const
    {
        return glm::vec4(r, g, b, a);
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
