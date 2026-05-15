#pragma once

#include "Core/Alloc.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Hasher.hpp"
#include "Core/Result.hpp"

#include <optional>

template <typename H, typename K, typename V>
struct Pair
{
    H hash;
    K key;
    V value;
};

template <typename K, typename V, typename H = Hasher<K>>
class HashMap
{
public:
    using Hash = uint64_t;
    using PairType = Pair<Hash, K, V>;

    HashMap()
        : m_size(0), m_capacity(0), m_pairs(nullptr)
    {
    }

    HashMap(const HashMap&) = delete;
    void operator==(const HashMap) = delete;

    ~HashMap()
    {
        if (m_pairs)
            destroy_array(m_pairs, m_size);
    }

    Result<void> put(const K& key, V value)
    {
        Hash hash = H{}(key);

        PairType *pair = TRY(insert(hash));
        pair->hash = hash;
        new (&pair->key) K(key);
        new (&pair->value) V(value);

        return Result<void>();
    }

    std::optional<V> get(const K& key) const
    {
        Hash hash = H{}(key);
        return find(hash);
    }

    bool contains(const K& key) const
    {
        Hash hash = H{}(key);
        size_t index;
        bool exact;
        bsearch(hash, index, exact);
        return exact;
    }

    void erase(const K& key)
    {
        Hash hash = H{}(key);

        size_t index;
        bool exact;
        bsearch(hash, index, exact);

        // no value for this key.
        if (!exact)
            return;

        m_pairs[index].~PairType();
        m_size -= 1;

        if (index == m_size - 1)
            return;

        std::memmove(m_pairs + index, m_pairs + index + 1, (m_size + 1) - index - 1);
    }

    void clear()
    {
        for (size_t i = 0; i < m_size; i++)
            m_pairs[i].~PairType();
        m_size = 0;
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

    std::optional<V> find(Hash hash) const
    {
        size_t index;
        bool exact;
        bsearch(hash, index, exact);

        if (exact)
            return m_pairs[index].value;
        return std::nullopt;
    }

    Result<PairType *> insert(Hash hash)
    {
        size_t index;
        bool exact;
        bsearch(hash, index, exact);

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
            if (index < m_size - 1)
                std::memmove((void *)(m_pairs + index + 1), (void *)(m_pairs + index), (m_size - index - 1) * sizeof(PairType));
            m_size += 1;
            return m_pairs + index;
        }
        else
        {
            size_t capacity = growth_factor(m_capacity);
            PairType *pairs = alloc_array_uninitialized<PairType>(capacity);
            if (!pairs)
                return Error(ErrorKind::BadDriver);

            if (index > 0)
                std::memmove((void *)pairs, (void *)m_pairs, index * sizeof(PairType));
            if (index < m_size - 1)
                std::memmove((void *)(pairs + index + 1), (void *)(m_pairs + index), (m_size - index - 1) * sizeof(PairType));

            destroy_array_nodestruct(m_pairs, 0);
            m_pairs = pairs;
            m_capacity = capacity;
            m_size += 1;

            return m_pairs + index;
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
        PairType *pair = nullptr;

        while (start <= end)
        {
            i = start + (end - start) / 2;
            pair = &m_pairs[i];

            if (hash == pair->hash)
            {
                index = i;
                exact = true;
                return;
            }
            else if (hash > pair->hash)
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

        if (hash > pair->hash)
            index += 1;
    }
};
