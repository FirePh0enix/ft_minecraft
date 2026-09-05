#pragma once

#include "Core/Stacktrace.hpp"

#include <format>
#include <print>

template <typename... Args>
static inline void assert_internal(bool condition, const char *expression_string, std::format_string<Args...> fmt, Args&&...args)
{
    if (!condition)
    {
        std::print(stderr, "Assertion `{}` failed: ", expression_string);
        std::println(stderr, fmt, std::forward<Args>(args)...);

        Stacktrace::record();
        Stacktrace::current().print(stderr);

        exit(1);
    }
}

// #ifndef NDEBUG

#define ASSERT(condition, format, ...) assert_internal(condition, #condition, format __VA_OPT__(, ) __VA_ARGS__)
#define ASSERT_V(...) ASSERT(__VA_ARGS__)

// #else

// #define ASSERT(condition, format, ...)
// #define ASSERT_V(condition, format, ...)

// #endif
