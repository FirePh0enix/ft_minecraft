#pragma once

#include "../lib/format.hpp"

template <typename T>
struct FormatBin
{
    T value;

    FormatBin(T value)
        : value(value)
    {
    }
};

template <typename T>
struct lib::formatter<FormatBin<T>>
{
    void format(const FormatBin<T>& value, lib::format_context& ctx) const
    {
        if (value.value > 1024 * 1024 * 1024)
        {
            return lib::format_to(ctx.out(), "{} GiB", (float)value.value / (float)(1024 * 1024 * 1024));
        }
        else if (value.value > 1024 * 1024)
        {
            return lib::format_to(ctx.out(), "{} MiB", (float)value.value / (float)(1024 * 1024));
        }
        else if (value.value > 1024)
        {
            return lib::format_to(ctx.out(), "{} KiB", (float)value.value / (float)(1024));
        }
        else
        {
            return lib::format_to(ctx.out(), "{} B", (float)value.value);
        }
    }
};
