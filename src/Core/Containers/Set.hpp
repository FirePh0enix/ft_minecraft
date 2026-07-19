#pragma once

#include "Core/Result.hpp"
#include "Core/Containers/Iterator.hpp"

#include <cstddef>

template <typename T>
class Set
{
public:
    Set()
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
    }

    Set(const Set& o)
        : m_size(o.m_size), m_capacity(o.m_size)
    {
        m_data = alloc_array_uninitialized<T>(o.m_size);
        for (size_t i = 0; i < m_size; i++)
            new (&m_data[i]) T(o.m_data[i]);
    }

    ~Set()
    {
        if (m_data)
            destroy_array(m_data, m_size);
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    void operator=(const Set& o)
    {
        if (m_data)
            destroy_array(m_data, m_size);

        if (o.m_size > 0)
        {
            m_size = o.m_size;
            m_capacity = o.m_size;
            m_data = alloc_array_uninitialized<T>(m_capacity);

            for (size_t i = 0; i < m_size; i++)
                new (&m_data[i]) T(o.m_data[i]);
        }
        else
        {
            m_size = 0;
            m_capacity = 0;
            m_data = nullptr;
        }
    }

    void put(const T& key)
    {
        T *p = insert(key);
        new (p) T(key);
    }

    bool contains(const T& key) const
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);
        return exact;
    }

    void erase(const T& key)
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        // no value for this key.
        if (!exact)
            return;

        m_data[index].~T();

        if (index < m_size - 1)
            std::memmove((void *)(m_data + index), (void *)(m_data + index + 1), (m_size - index) * sizeof(T));

        m_size -= 1;
    }

    void clear()
    {
        destroy_array(m_data, m_size);
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }

    ForwardIterator<T> begin() const { return ForwardIterator<T>(m_data); }
    ForwardIterator<T> end() const { return ForwardIterator<T>(m_data + m_size); }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

private:
    T *m_data;
    size_t m_size;
    size_t m_capacity;

    T *insert(const T& key)
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        if (exact)
            return &m_data[index];

        if (m_capacity == 0)
        {
            size_t capacity = initial_capacity;
            T *pairs = alloc_array_uninitialized<T>(capacity);

            m_data = pairs;
            m_capacity = capacity;
            m_size = 1;

            return m_data;
        }
        else if (m_size + 1 <= m_capacity)
        {
            if (m_size > 0 && index < m_size)
                std::memmove((void *)(m_data + index + 1), (void *)(m_data + index), (m_size - index) * sizeof(T));
            m_size += 1;
            return m_data + index;
        }
        else
        {
            size_t capacity = growth_factor(m_capacity);
            T *pairs = alloc_array_uninitialized<T>(capacity);

            if (index > 0)
                std::memmove((void *)pairs, (void *)m_data, index * sizeof(T));
            if (index < m_size)
                std::memmove((void *)(pairs + index + 1), (void *)(m_data + index), (m_size - index) * sizeof(T));

            destroy_array_nodestruct(m_data, 0);
            m_data = pairs;
            m_capacity = capacity;
            m_size += 1;

            return m_data + index;
        }
    }

    void bsearch(const T& key, size_t& index, bool& exact) const
    {
        if (m_size == 0)
        {
            index = 0;
            exact = false;
            return;
        }

        size_t start = 0;
        size_t end = m_size - 1;
        size_t i = 0;
        T *p = nullptr;

        while (start <= end)
        {
            i = start + (end - start) / 2;
            p = &m_data[i];

            if (key == *p)
            {
                index = i;
                exact = true;
                return;
            }
            else if (key > *p)
            {
                if (i == m_size - 1)
                    break;
                start = i + 1;
            }
            else
            {
                if (i == 0)
                    break;
                end = i - 1;
            }
        }

        index = i;
        exact = false;

        if (key > *p)
            index += 1;
    }
};
