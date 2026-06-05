#pragma once

#include "Core/Containers/View.hpp"
#include "Core/Result.hpp"

template <typename K, typename V>
struct MapPair
{
    K key;
    V value;
};

inline bool operator<(const glm::ivec3& a, const glm::ivec3& b)
{
    if (a.x != b.x)
        return a.x < b.x;
    if (a.y != b.y)
        return a.y < b.y;
    return a.z < b.z;
}

inline bool operator>(const glm::ivec3& a, const glm::ivec3& b)
{
    if (a.x != b.x)
        return a.x > b.x;
    if (a.y != b.y)
        return a.y > b.y;
    return a.z > b.z;
}

template <typename K, typename V>
class Map
{
public:
    using PairType = MapPair<K, V>;

    Map()
        : m_size(0), m_capacity(0), m_pairs(nullptr)
    {
    }

    Map(const Map& o)
        : m_size(o.m_size), m_capacity(o.m_size)
    {
        if (o.m_size > 0)
        {
            m_pairs = alloc_array_uninitialized<PairType>(o.m_size);
            for (size_t i = 0; i < m_size; i++)
                new (&m_pairs[i]) PairType(o.m_pairs[i]);
        }
        else
        {
            m_pairs = nullptr;
        }
    }

    ~Map()
    {
        if (m_pairs)
            destroy_array(m_pairs, m_size);
        m_pairs = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    void operator=(const Map& o)
    {
        if (m_pairs)
            destroy_array(m_pairs, m_size);

        if (o.m_size > 0)
        {
            m_size = o.m_size;
            m_capacity = o.m_size;
            m_pairs = alloc_array_uninitialized<PairType>(m_capacity);

            for (size_t i = 0; i < m_size; i++)
                new (&m_pairs[i]) PairType(o.m_pairs[i]);
        }
        else
        {
            m_size = 0;
            m_capacity = 0;
            m_pairs = nullptr;
        }
    }

    void put(const K& key, V value)
    {
        bool exist;
        PairType *pair = insert(key, exist);
        if (!exist)
        {
            new (&pair->key) K(key);
            new (&pair->value) V(value);
        }
        else
        {
            new (&pair->value) V(value);
        }
    }

    Option<V> get(const K& key) const
    {
        return find(key);
    }

    Option<V *> get_ptr(const K& key)
    {
        return find_ptr(key);
    }

    Option<const V *> get_ptr(const K& key) const
    {
        return find_const_ptr(key);
    }

    V *get_or_put(const K& key, V value)
    {
        bool exist;
        PairType *pair = insert(key, exist);
        if (!exist)
        {
            new (&pair->key) K(key);
            new (&pair->value) V(value);
        }
        return &pair->value;
    }

    bool contains(const K& key) const
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);
        return exact;
    }

    void erase(const K& key)
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        // no value for this key.
        if (!exact)
            return;

        m_pairs[index].~PairType();

        if (index < m_size - 1)
            std::memmove((void *)(m_pairs + index), (void *)(m_pairs + index + 1), (m_size - index) * sizeof(PairType));

        m_size -= 1;
    }

    void clear()
    {
        destroy_array(m_pairs, m_size);
        m_pairs = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    View<PairType> pairs() const { return View(m_pairs, m_size); }
    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

    ForwardIterator<PairType> begin() const { return ForwardIterator<PairType>(m_pairs); }
    ForwardIterator<PairType> end() const { return ForwardIterator<PairType>(m_pairs + m_size); }

private:
    size_t m_size;
    size_t m_capacity;
    PairType *m_pairs;

    Option<V> find(const K& key) const
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        if (exact)
            return m_pairs[index].value;
        return None;
    }

    Option<V *> find_ptr(const K& key)
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        if (exact)
            return &m_pairs[index].value;
        return None;
    }

    Option<const V *> find_const_ptr(const K& key) const
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        if (exact)
            return &m_pairs[index].value;
        return None;
    }

    PairType *insert(const K& key, bool& exist)
    {
        size_t index;
        bool exact;
        bsearch(key, index, exact);

        exist = exact;
        if (exact)
            return &m_pairs[index];

        if (m_capacity == 0)
        {
            size_t capacity = initial_capacity;
            PairType *pairs = alloc_array_uninitialized<PairType>(capacity);

            m_pairs = pairs;
            m_capacity = capacity;
            m_size = 1;

            return m_pairs;
        }
        else if (m_size + 1 <= m_capacity)
        {
            if (m_size > 0 && index < m_size)
                std::memmove((void *)(m_pairs + index + 1), (void *)(m_pairs + index), (m_size - index) * sizeof(PairType));
            m_size += 1;
            return m_pairs + index;
        }
        else
        {
            size_t capacity = growth_factor(m_capacity);
            PairType *pairs = alloc_array_uninitialized<PairType>(capacity);

            if (index > 0)
                std::memmove((void *)pairs, (void *)m_pairs, index * sizeof(PairType));
            if (index < m_size)
                std::memmove((void *)(pairs + index + 1), (void *)(m_pairs + index), (m_size - index) * sizeof(PairType));

            destroy_array_nodestruct(m_pairs, 0);
            m_pairs = pairs;
            m_capacity = capacity;
            m_size += 1;

            return m_pairs + index;
        }
    }

    void bsearch(const K& key, size_t& index, bool& exact) const
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
        PairType *pair = nullptr;

        while (start <= end)
        {
            i = start + (end - start) / 2;
            pair = &m_pairs[i];

            if (key == pair->key)
            {
                index = i;
                exact = true;
                return;
            }
            else if (key > pair->key)
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

        if (key > pair->key)
            index += 1;
    }
};
