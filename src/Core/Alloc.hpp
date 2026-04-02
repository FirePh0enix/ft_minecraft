#pragma once

#include <cstdlib>

typedef void *(*MallocHook)(size_t);
typedef void (*FreeHook)(void *);

namespace core
{
extern MallocHook malloc_func;
extern FreeHook free_func;
}; // namespace core

/**
 * Allocate memory. It is similar to `new` except that it returns `nullptr` if the allocation fails.
 */
template <typename T, typename... Args>
T *alloc(Args&&...args)
{
    T *obj = static_cast<T *>(core::malloc_func(sizeof(T)));
    if (obj == nullptr)
        return nullptr;

    new (obj) T(args...);
    return obj;
}

template <typename T>
T *alloc_array_uninitialized(size_t n)
{
    return static_cast<T *>(core::malloc_func(sizeof(T) * n));
}

template <typename T>
T *alloc_array(size_t n)
{
    T *ptr = alloc_array_uninitialized<T>(n);
    if (!ptr)
        return nullptr;
    for (size_t i = 0; i < n; i++)
        new (ptr + i) T();
    return ptr;
}

template <typename T>
void destroy(T *obj)
{
    obj->~T();
    core::free_func(obj);
}

template <typename T>
void destroy_array_nodestruct(T *array, size_t n)
{
    (void)n;
    core::free_func(array);
}

template <typename T>
void destroy_array(T *array, size_t n)
{
    for (size_t i = 0; i < n; i++)
        array[i].~T();
    destroy_array_nodestruct(array, n);
}
