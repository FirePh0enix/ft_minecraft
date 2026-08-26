#pragma once

#include <cstdint>

enum class SizeType
{
    Px,
    Percent,
};

struct Size
{
    SizeType type = SizeType::Px;
    union
    {
        int32_t px = 0;
        float percent;
    } data;

    static Size px(int32_t value)
    {
        return Size{.type = SizeType::Px, .data{.px = value}};
    }

    static Size percent(float value)
    {
        return Size{.type = SizeType::Percent, .data{.percent = value}};
    }
};

struct Point
{
    Size x;
    Size y;

    Point()
    {
    }

    Point(Size scalar)
        : x(scalar), y(scalar)
    {
    }

    Point(Size x, Size y)
        : x(x), y(y)
    {
    }
};
