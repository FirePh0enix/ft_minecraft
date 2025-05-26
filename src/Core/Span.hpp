#pragma once

template <typename T>
class Span
{
public:
    class Iterator
    {
    public:
        using difference_type = ssize_t;
        using value_type = T;
        using pointer = const T *;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;

        Iterator(const T *ptr)
            : m_ptr(ptr)
        {
        }

        Iterator& operator++()
        {
            m_ptr += 1;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator val = *this;
            m_ptr += 1;
            return val;
        }

        bool operator==(Iterator other)
        {
            return m_ptr == other.m_ptr;
        }

        bool operator!=(Iterator other)
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

    Span()
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

    Span(const std::initializer_list<T>& list)
        : m_data(list.begin()), m_size(list.size())
    {
    }

    const T& operator[](size_t size) const
    {
        return m_data[size];
    }

    T& operator[](size_t size)
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

    inline Iterator begin() const
    {
        return Iterator(data());
    }

    inline Iterator end() const
    {
        return Iterator(data() + size());
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
