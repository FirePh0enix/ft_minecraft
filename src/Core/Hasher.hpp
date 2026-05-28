#pragma once

#include <cstddef>
#include <cstdint>

template <typename T>
struct Hasher;

template <>
struct Hasher<unsigned char>
{
    uint64_t operator()(const unsigned char& i)
    {
        return (uint64_t)i;
    }
};

template <>
struct Hasher<int>
{
    uint64_t operator()(const int& i)
    {
        return *(unsigned int *)(int *)&i;
    }
};

template <>
struct Hasher<unsigned int>
{
    uint64_t operator()(const unsigned int& i)
    {
        return (uint64_t)i;
    }
};

template <>
struct Hasher<int64_t>
{
    uint64_t operator()(const int64_t& i)
    {
        return *(uint64_t *)&i;
    }
};

template <>
struct Hasher<uint64_t>
{
    uint64_t operator()(const uint64_t& i)
    {
        return i;
    }
};

template <typename T>
struct Hasher<T *>
{
    uint64_t operator()(T *const& i)
    {
        return (uint64_t)(size_t)i;
    }
};
