#pragma once

#include <format>

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
struct std::formatter<FormatBin<T>> : std::formatter<std::string>
{
    auto format(const FormatBin<T>& value, std::format_context& ctx) const
    {
        if (value.value > 1024 * 1024 * 1024)
        {
            return std::format_to(ctx.out(), "{} GiB", (float)value.value / (float)(1024 * 1024 * 1024));
        }
        else if (value.value > 1024 * 1024)
        {
            return std::format_to(ctx.out(), "{} MiB", (float)value.value / (float)(1024 * 1024));
        }
        else if (value.value > 1024)
        {
            return std::format_to(ctx.out(), "{} KiB", (float)value.value / (float)(1024));
        }
        else
        {
            return std::format_to(ctx.out(), "{} B", (float)value.value);
        }
    }
};
