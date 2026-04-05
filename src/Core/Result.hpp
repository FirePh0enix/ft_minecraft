#pragma once

#include "Core/Error.hpp"

#include <optional>

template <typename T, typename E = Error>
class [[nodiscard]] Result
{
public:
    Result(const T& value)
        : m_value(value)
    {
    }

    Result(const E& error)
        : m_error(error)
    {
    }

    inline const T *operator->() const
    {
        return &m_value.value();
    }

    inline T *operator->()
    {
        return &m_value.value();
    }

    inline operator bool() const
    {
        return has_value();
    }

    inline bool has_value() const
    {
        return m_value.has_value();
    }

    inline bool has_error() const
    {
        return m_error.has_value();
    }

    inline const T& value() const { return m_value.value(); }
    inline T& value() { return m_value.value(); }

    inline T value_or(const T& default_value)
    {
        return has_value() ? value() : default_value;
    }

    inline const E& error() const { return m_error.value(); }
    inline E& error() { return m_error.value(); }

private:
    std::optional<T> m_value;
    std::optional<E> m_error;
};

template <typename E>
class [[nodiscard]] Result<void, E>
{
public:
    Result(const E& error)
        : m_error(error)
    {
    }

    Result()
    {
    }

    inline operator bool() const
    {
        return has_value();
    }

    inline bool has_value() const
    {
        return !m_error.has_value();
    }

    inline bool has_error() const
    {
        return !has_value();
    }

    inline void value() const
    {
    }

    inline const E& error() const
    {
        return *m_error;
    }

    inline E& error()
    {
        return *m_error;
    }

private:
    std::optional<E> m_error;
};

// Inspired by Ladybird `TRY` and `MUST` macro. This depends on (statement expression)[1] which is non standard but implemented by gcc and clang.
//
// [1]: https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html

#define TRY(...)                     \
    ({                               \
        auto&& _tmp = (__VA_ARGS__); \
        (void)_tmp;                  \
        if (!_tmp.has_value())       \
            return _tmp.error();     \
        _tmp.value();                \
    })

#define EXPECT(EXPECTED)              \
    ({                                \
        auto __result = EXPECTED;     \
        if (!__result.has_value())    \
        {                             \
            __result.error().print(); \
            std::abort();             \
        }                             \
        __result.value();             \
    })
