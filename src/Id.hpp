#pragma once

#include "Core/Hasher.hpp"

template <typename T>
struct Id
{
    explicit constexpr Id(uint16_t value)
        : value(value)
    {
    }

    constexpr Id()
        : value(0)
    {
    }

    bool operator==(const Id& k) const { return value == k.value; }

    constexpr bool valid() const { return value != 0; }

    uint16_t value;
};

template <typename T>
struct Hasher<Id<T>>
{
    uint64_t operator()(const Id<T>& i)
    {
        return i.value;
    }
};
