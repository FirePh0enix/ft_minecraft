#pragma once

#include "Core/StringView.hpp"

#include <cstdint>

constexpr uint32_t hash_fnv32(StringView s)
{
    const uint32_t fnv_32_prime = 0x01000193;
    uint32_t h = 0x811c9dc5;

    size_t i = 0;
    const size_t len = s.size();

    while (i < len)
    {
        h ^= s.data()[i++];
        h *= fnv_32_prime;
    }

    return h;
}
