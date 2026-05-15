#pragma once

#include <cstdint>

template <typename T>
struct Hasher;

template <>
struct Hasher<int>
{
    uint64_t operator()(const int& i)
    {
        return *(unsigned int *)(int *)&i | (0 << 31);
    }
};
