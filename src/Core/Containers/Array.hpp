#pragma once

#include "Core/Assert.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Definitions.hpp"

#include <cstddef>
#include <initializer_list>

template <typename T, const size_t _S>
class Array
{
public:
    Array()
        : m_data{}
    {
    }

    template <typename... Args>
    Array(const std::initializer_list<T>& list)
    {
        for (size_t i = 0; i < list.size(); i++)
            m_data[i] = *(list.begin() + i);
    }

    constexpr const T& operator[](size_t index) const
    {
        ASSERT_V(index <= _S, "");
        return m_data[index];
    }

    constexpr T& operator[](size_t index)
    {
        ASSERT_V(index <= _S, "");
        return m_data[index];
    }

    ALWAYS_INLINE T *data() { return m_data; }
    ALWAYS_INLINE const T *data() const { return m_data; }

    ALWAYS_INLINE constexpr size_t size() const { return _S; }

    ForwardIterator<T> begin() const
    {
        return ForwardIterator<T>(data());
    }

    ForwardIterator<T> end() const
    {
        return ForwardIterator<T>(&data()[_S]);
    }

private:
    T m_data[_S];
};
