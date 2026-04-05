#pragma once

#include <cstdint>
#include <iterator>

template <typename T>
class ForwardIterator
{
public:
    // NOLINTBEGIN those declarations are required by C++ to create an iterator class.
    using difference_type = int64_t;
    using value_type = T;
    using pointer = const T *;
    using reference = const T&;
    using iterator_category = std::forward_iterator_tag;
    // NOLINTEND

    ForwardIterator(const T *ptr)
        : m_ptr((T *)ptr)
    {
    }

    ForwardIterator& operator++()
    {
        m_ptr += 1;
        return *this;
    }

    ForwardIterator operator++(int)
    {
        ForwardIterator val = *this;
        m_ptr += 1;
        return val;
    }

    bool operator==(ForwardIterator other)
    {
        return m_ptr == other.m_ptr;
    }

    bool operator!=(ForwardIterator other)
    {
        return !(*this == other);
    }

    T& operator*()
    {
        return *m_ptr;
    }

    difference_type operator-(ForwardIterator other)
    {
        return m_ptr - other.m_ptr;
    }

private:
    T *m_ptr;
};
