#pragma once

#include "Core/Format.hpp"
#include "Core/Print.hpp"

template <typename... Args>
static inline void __assert_internal(bool condition, const char *expression_string, FormatString<Args...> fmt, Args&&...args)
{
    if (!condition)
    {
        print("Assertion `{}` failed: ", expression_string);
        println(fmt, std::forward<Args>(args)...);
        std::abort();
    }
}

#ifndef NDEBUG

#define ASSERT(condition, format) __assert_internal(condition, #condition, format)
#define ASSERT_V(condition, format, ...) __assert_internal(condition, #condition, format, __VA_ARGS__)

#else

#define ASSERT(condition, format)
#define ASSERT_V(condition, format, ...)

#endif
