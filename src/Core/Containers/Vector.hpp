#pragma once

#include <cstddef>

#include "Core/Alloc.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Definitions.hpp"
#include "Core/Error.hpp"
#include "Core/Result.hpp"

/**
 * @brief Dynamic array with fallible memory allocation.
 */
template <typename T>
class Vector
{
public:
    Vector()
        : m_data(nullptr), m_capacity(0), m_size(0)
    {
    }

    ~Vector()
    {
        if (!m_data)
            destroy_array<T>(m_data, m_size);
    }

    ALWAYS_INLINE T *data() { return m_data; }
    ALWAYS_INLINE const T *data() const { return m_data; }

    ALWAYS_INLINE size_t size() const { return m_size; }
    ALWAYS_INLINE size_t capacity() const { return m_capacity; }

    Result<> append(const T&& value)
    {
        YEET(grow_if_needed());
        m_data[m_size++] = value;
    }

    ForwardIterator<T> begin()
    {
        return ForwardIterator<T>(data());
    }

    ForwardIterator<T> end()
    {
        return ForwardIterator<T>(data() + m_size);
    }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

private:
    T *m_data;
    size_t m_capacity;
    size_t m_size;

    Result<> grow_if_needed()
    {
        if (m_capacity == 0)
        {
            m_capacity = initial_capacity;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
                return Error(ErrorKind::OutOfMemory);
        }
        else if (m_size == m_capacity)
        {
            size_t new_capacity = growth_factor(m_capacity);
            T *new_data = alloc_array_uninitialized<T>(new_capacity);
            if (!new_data)
                return Error(ErrorKind::OutOfMemory);
            memcpy(new_data, m_data, new_capacity * sizeof(T));

            destroy_array_nodestruct<T>(m_data, m_size);
            m_data = new_data;
            m_capacity = new_capacity;
        }
    }
};
