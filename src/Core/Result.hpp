#pragma once

#include "Core/Error.hpp"

#include <variant>

template <typename T = char, typename E = Error>
class Result
{
public:
    Result(const T& value)
        : m_variant(value)
    {
    }

    Result(const E& error)
        : m_variant(error)
    {
    }

    inline const T *operator->() const
    {
        return &std::get<T>(m_variant);
    }

    inline T *operator->()
    {
        return &std::get<T>(m_variant);
    }

    inline operator bool() const
    {
        return has_value();
    }

    inline bool has_value() const
    {
        return std::holds_alternative<T>(m_variant);
    }

    inline bool has_error() const
    {
        return std::holds_alternative<E>(m_variant);
    }

    inline const T& value() const
    {
        return std::get<T>(m_variant);
    }

    inline T& value()
    {
        return std::get<T>(m_variant);
    }

    inline T value_or(const T& default_value)
    {
        return has_value() ? value() : default_value;
    }

    inline const E& error() const
    {
        return std::get<E>(m_variant);
    }

    inline E& error()
    {
        return std::get<E>(m_variant);
    }

private:
    std::variant<std::monostate, T, E> m_variant;
};

#define YEET(EXPECTED)               \
    do                               \
    {                                \
        auto __result = EXPECTED;    \
        if (!__result.has_value())   \
            return __result.error(); \
    } while (0)

#define EXPECT(EXPECTED)              \
    do                                \
    {                                 \
        auto __result = EXPECTED;     \
        if (!__result.has_value())    \
        {                             \
            __result.error().print(); \
            exit(1);                  \
        }                             \
    } while (0)
