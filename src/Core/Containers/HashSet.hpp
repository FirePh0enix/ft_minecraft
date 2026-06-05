#pragma once

#include "Core/Result.hpp"

#include <cstddef>

template <typename T>
class HashSet
{
public:
    using Hash = uint64_t;

    HashSet()
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
    }

    HashSet(const HashSet& o)
        : m_size(o.m_size), m_capacity(o.m_size)
    {
        m_data = alloc_array_uninitialized<Hash>(o.m_size);
        for (size_t i = 0; i < m_size; i++)
            new (&m_data[i]) T(o.m_data[i]);
    }

    ~HashSet()
    {
        if (m_data)
            destroy_array(m_data, m_size);
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    void operator=(const HashSet& o)
    {
        if (m_data)
            destroy_array(m_data, m_size);

        if (o.m_size > 0)
        {
            m_size = o.m_size;
            m_capacity = o.m_size;
            m_data = alloc_array_uninitialized<Hash>(m_capacity);

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
        Hash hash = Hasher<T>{}(key);

        Hash *p = insert(hash);
        new (p) Hash(hash);
    }

    bool contains(const T& key) const
    {
        Hash hash = Hasher<T>{}(key);

        size_t index;
        bool exact;
        bsearch(hash, index, exact);
        return exact;
    }

    void erase(const T& key)
    {
        Hash hash = Hasher<T>{}(key);

        size_t index;
        bool exact;
        bsearch(hash, index, exact);

        // no value for this key.
        if (!exact)
            return;

        m_data[index].~Hash();

        if (index < m_size - 1)
            std::memmove((void *)(m_data + index), (void *)(m_data + index + 1), (m_size - index) * sizeof(T));

        m_size -= 1;
    }

    Option<size_t> index_of(const T& key)
    {
        Hash hash = Hasher<T>{}(key);

        size_t index;
        bool exact;
        bsearch(hash, index, exact);

        if (!exact)
            return None;
        return index;
    }

    void clear()
    {
        if (m_data)
        {
            destroy_array(m_data, m_size);
            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
        }
    }

    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

private:
    Hash *m_data;
    size_t m_size;
    size_t m_capacity;

    Hash *insert(Hash hash)
    {
        size_t index;
        bool exact;
        bsearch(hash, index, exact);

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

    void bsearch(Hash hash, size_t& index, bool& exact) const
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

            if (hash == *p)
            {
                index = i;
                exact = true;
                return;
            }
            else if (hash > *p)
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

        if (hash > *p)
            index += 1;
    }
};
