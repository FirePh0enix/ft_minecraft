#pragma once

template <typename T>
class ForwardIterator
{
public:
    using difference_type = ssize_t;
    using value_type = T;
    using pointer = const T *;
    using reference = const T&;
    using iterator_category = std::forward_iterator_tag;

    ForwardIterator(const T *ptr)
        : m_ptr(ptr)
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

    T operator*()
    {
        return *m_ptr;
    }

private:
    const T *m_ptr;
};
