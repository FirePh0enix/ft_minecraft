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

    explicit constexpr Id(uint32_t id)
        : hash(id), str(nullptr)
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
