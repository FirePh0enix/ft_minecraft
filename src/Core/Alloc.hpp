#pragma once

#include <cstdlib>

/**
 * Allocate memory. It is similar to `new` except that it returns `nullptr` if the allocation fails.
 */
template <typename T, typename... Args>
T *alloc(Args&&...args)
{
    T *obj = static_cast<T *>(malloc(sizeof(T)));
    if (obj == nullptr)
        return nullptr;

    new (obj) T(args...);
    return obj;
}

template <typename T>
T *alloc_n(size_t n)
{
    return static_cast<T *>(malloc(sizeof(T) * n));
}

template <typename T>
void destroy(T *obj)
{
    T::~T(obj);
    free(obj);
}

template <typename T>
void destroy_n(T *array)
{
    free(array);
}
