#pragma once

#include "Core/Format.hpp"
#include "Core/Print.hpp"
#include "Core/Stacktrace.hpp"

template <typename... Args>
static inline void assert_internal(bool condition, const char *expression_string, FormatString<Args...> fmt, Args&&...args)
{
    if (!condition)
    {
        print(stderr, "Assertion `{}` failed: ", expression_string);
        println(stderr, fmt, std::forward<Args>(args)...);

        Stacktrace::record();
        Stacktrace::current().print(stderr);

        std::abort();
    }
}

#ifndef NDEBUG

#define ASSERT(condition, format, ...) assert_internal(condition, #condition, format __VA_OPT__(, ) __VA_ARGS__)
#define ASSERT_V(...) ASSERT(__VA_ARGS__)

#else

#define ASSERT(condition, format, ...)
#define ASSERT_V(condition, format, ...)

#endif
