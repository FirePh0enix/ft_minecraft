#pragma once

#include "Core/Containers/Iterator.hpp"
#include "Core/Result.hpp"

#include <cstddef>
#include <utility>

template <typename T>
class LocalVector
{
public:
    LocalVector()
        : m_data(nullptr), m_size(0), m_capacity(0)
    {
    }

    ~LocalVector()
    {
        destroy_array(m_data, m_size);
    }

    ALWAYS_INLINE T *data() { return m_data; }
    ALWAYS_INLINE const T *data() const { return m_data; }

    ALWAYS_INLINE size_t size() const { return m_size; }
    ALWAYS_INLINE size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    Result<void> append(const T& value)
    {
        TRY(grow_if_needed());

        new (m_data + m_size) T(value);
        m_size += 1;

        return Result<void>();
    }

    template <typename... Args>
    Result<void> emplace(Args&&...args)
    {
        TRY(grow_if_needed());

        new (m_data + m_size) T(std::forward<Args>(args)...);
        m_size += 1;

        return Result<void>();
    }

    const T& get_unchecked(size_t index) const { return m_data[index]; }
    T& get_unchecked(size_t index) { return m_data[index]; }

    ForwardIterator<T> begin() const
    {
        return ForwardIterator<T>(data());
    }

    ForwardIterator<T> end() const
    {
        return ForwardIterator<T>(&data()[m_size]);
    }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

private:
    T *m_data;
    size_t m_size;
    size_t m_capacity;

    Result<void> grow_if_needed()
    {
        if (m_capacity == 0)
        {
            m_capacity = initial_capacity;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
            {
                return Error(ErrorKind::OutOfMemory);
            }
        }
        else if (m_size == m_capacity)
        {
            size_t new_capacity = growth_factor(m_capacity);
            TRY(grow_to(m_capacity, new_capacity));
        }

        return Result<void>();
    }

    Result<void> ensure_at_least(size_t aditional_size)
    {
        if (m_capacity == 0)
        {
            m_capacity = aditional_size;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
            {
                return Error(ErrorKind::OutOfMemory);
            }
        }
        else if (m_size == m_capacity)
        {
            size_t new_capacity = m_size + aditional_size;
            TRY(grow_to(m_capacity, new_capacity));
        }

        return Result<void>();
    }

    Result<void> grow_to(size_t old_capacity, size_t new_capacity)
    {
        T *new_data = alloc_array_uninitialized<T>(new_capacity);
        if (!new_data)
            return Error(ErrorKind::OutOfMemory);

        // perform a deep copy.
        // TODO: uncessary, elements are moved not copied. memcpy should be enough.
        // for (size_t i = 0; i < m_size; i++)
        //     new (new_data + i) T(m_data[i]);
        std::memcpy((void *)new_data, m_data, m_size * sizeof(T));

        destroy_array_nodestruct(m_data, old_capacity);
        m_data = new_data;
        m_capacity = new_capacity;

        return Result<void>();
    }
};
