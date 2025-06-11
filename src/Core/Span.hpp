#pragma once

#include "Core/Iterator.hpp"

template <typename T>
class Span
{
public:
    Span()
    {
    }

    Span(const T& value)
        : m_data(&value), m_size(1)
    {
    }

    Span(const T *data, size_t size)
        : m_data(data), m_size(size)
    {
    }

    Span(const std::vector<T>& vector)
        : m_data(vector.data()), m_size(vector.size())
    {
    }

    template <const size_t size>
    Span(const std::array<T, size>& array)
        : m_data(array.data()), m_size(array.size())
    {
    }

    const T& operator[](size_t size) const
    {
        return m_data[size];
    }

    const T& operator[](size_t size)
    {
        return m_data[size];
    }

    inline const T *data() const
    {
        return m_data;
    }

    inline size_t size() const
    {
        return m_size;
    }

    inline ForwardIterator<T> begin() const
    {
        return ForwardIterator<T>(data());
    }

    inline ForwardIterator<T> end() const
    {
        return ForwardIterator<T>(data() + size());
    }

    Span<uint8_t> as_bytes() const
    {
        return Span<uint8_t>((const uint8_t *)m_data, m_size * sizeof(T));
    }

    std::vector<T> to_vector() const
    {
        std::vector<T> vec;
        vec.insert(vec.begin(), begin(), end());
        return vec;
    }

private:
    const T *m_data;
    size_t m_size;
};
