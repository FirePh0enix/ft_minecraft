#pragma once

#include "Core/Hash.hpp"

#include <cstdint>
#include <string_view>

template <typename T>
struct Id
{
    explicit constexpr Id(std::string_view name)
        : hash(hash_fnv32(name)), str(name.data())
    {
    }

    constexpr Id()
        : hash(0), str(nullptr)
    {
    }

    bool operator==(const Id& k) const { return hash == k.hash; }
    bool operator>(const Id& k) const { return hash > k.hash; }
    bool operator<(const Id& k) const { return hash < k.hash; }

    constexpr bool valid() const { return str != nullptr; }

    uint32_t hash;
    const char *str;
};

template <typename T>
struct RuntimeId
{
    uint16_t value;

    explicit constexpr RuntimeId(uint16_t value)
        : value(value)
    {
    }

    constexpr RuntimeId()
        : value(0)
    {
    }

    bool operator==(const RuntimeId& k) const { return value == k.value; }
    bool operator>(const RuntimeId& k) const { return value > k.value; }
    bool operator<(const RuntimeId& k) const { return value < k.value; }

    constexpr bool valid() const { return value != 0; }
};
