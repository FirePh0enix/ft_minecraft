#pragma once

#include "Core/Assert.hpp"

struct NoneType
{
};

static constexpr NoneType None;

template <typename T>
class [[nodiscard]] __attribute__((aligned(alignof(T)))) Option
{
public:
    Option()
        : m_has_value(false)
    {
    }

    Option(const T& value)
        : m_has_value(true)
    {
        new ((T *)m_data) T(value);
    }

    Option(NoneType)
        : m_has_value(false)
    {
    }

    Option(const Option& o)
        : m_has_value(o.m_has_value)
    {
        if (m_has_value)
            new ((T *)m_data) T(*(T *)o.m_data);
    }

    ~Option()
    {
        if (m_has_value)
            ((T *)m_data)->~T();
    }

    Option& operator=(const Option& o)
    {
        if (m_has_value)
            ((T *)m_data)->~T();

        m_has_value = o.m_has_value;
        if (m_has_value)
            new ((T *)m_data) T(*(T *)o.m_data);

        return *this;
    }

    bool operator==(const Option& o) const
    {
        return (has_value() == o.has_value()) && (has_value() ? value() == o.value() : true);
    }

    bool operator==(const T& v) const
    {
        return has_value() && v == value();
    }

    operator int() const = delete;
    operator bool() const { return has_value(); }

    inline bool has_value() const { return m_has_value; }

    const T& value() const
    {
        ASSERT_V(has_value(), "Trying to access Option");
        return *(const T *)m_data;
    }

    T& value()
    {
        ASSERT_V(has_value(), "Trying to access Option");
        return *(T *)m_data;
    }

    T value_or(const T& other) const
    {
        if (m_has_value)
            return value();
        return other;
    }

    /**
     * Convert Option<T> to Option<B> by calling `f`.
     */
    template <typename B>
    Option<B> map(std::function<B(const T&)> f) const
    {
        if (!m_has_value)
            return None;
        return Option<B>(f(value()));
    }

private:
    char m_data[sizeof(T)]{};
    bool m_has_value = true;
};

/**
 * Specialization of Option for pointers. It is guaranteed that this type is the same layout as a pointer, where
 * a nullptr pointer represent an optional with no value.
 */
template <typename T>
class [[nodiscard]] Option<T *>
{
public:
    Option()
        : m_value(nullptr)
    {
    }

    Option(T *value)
        : m_value(value)
    {
    }

    Option(NoneType)
        : m_value(nullptr)
    {
    }

    bool operator==(const Option& o) const
    {
        return (has_value() == o.has_value()) && (has_value() ? value() == o.value() : true);
    }

    bool operator==(const T *& v) const
    {
        return has_value() && v == value();
    }

    operator bool() const { return has_value(); }

    inline bool has_value() const { return m_value != nullptr; }

    const T *& value() const
    {
        ASSERT_V(has_value(), "");
        return m_value;
    }

    T *& value()
    {
        ASSERT_V(has_value(), "");
        return m_value;
    }

    /**
     * Convert Option<T> to Option<B> by calling `f`.
     */
    template <typename B>
    Option<B> map(std::function<B(const T *&)> f)
    {
        if (!has_value())
            return None;
        return Option<T>(f(value()));
    }

private:
    T *m_value;
};

static_assert(sizeof(void *) == sizeof(Option<void *>));
