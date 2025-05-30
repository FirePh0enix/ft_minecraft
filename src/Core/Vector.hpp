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

template <typename T>
class Vector
{
public:
    Vector()
        : m_data(nullptr), m_capacity(0), m_size(0)
    {
    }

    const T& operator[](size_t index) const
    {
        return m_data[index];
    }

    T& operator[](size_t index)
    {
        return m_data[index];
    }

    ForwardIterator<T> begin()
    {
        return ForwardIterator<T>(m_data);
    }

    ForwardIterator<T> end()
    {
        return ForwardIterator<T>(m_data + m_size);
    }

    inline const T *data() const
    {
        return m_data;
    }

    inline T *data()
    {
        return m_data;
    }

    inline size_t size() const
    {
        return m_size;
    }

    /**
     * Reset the size to `0` and keep the internal capacity.
     */
    void clear_retain_capacity()
    {
        m_size = 0;
    }

    void push_back(const T&& value)
    {
        ZoneScoped;

        if (m_capacity == m_size)
        {
            reserve_more(5, {});
        }

        m_data[m_size] = value;
        m_size += 1;
    }

    void reserve_more(size_t n)
    {
        reserve_more(n, {});
    }

    void reserve_more(size_t n, const T&& value)
    {
        const size_t new_capacity = m_capacity + n;
        T *new_data = new T[new_capacity](value);
        std::memcpy(new_data, m_data, sizeof(T) * m_capacity);

        delete[] m_data;
        m_capacity = new_capacity;
        m_data = new_data;
    }

private:
    T *m_data;
    size_t m_capacity;
    size_t m_size;
};
