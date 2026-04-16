#pragma once

#include <cstddef>

#include "Core/Alloc.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Definitions.hpp"
#include "Core/Error.hpp"
#include "Core/Result.hpp"

/**
 *  @brief Dynamic array with fallible memory allocation.
 */
template <typename T>
class Vector
{
public:
    /**
     *  @brief Create a new `Vector` filled with a variable number of arguments.
     */
    template <typename... Args>
    static Result<Vector> create(Args&&...args)
    {
        Vector v;
        TRY(v.reserve(sizeof...(args)));
        TRY(v.append(args), ...);
        return v;
    }

    Vector()
        : m_data(nullptr), m_references(nullptr), m_size(0), m_capacity(0)
    {
    }

    Vector(const Vector& vec)
        : m_data(vec.m_data), m_references(vec.m_references), m_size(vec.m_size), m_capacity(vec.m_capacity)
    {
        if (m_references)
            *m_references += 1;
    }

    ~Vector()
    {
        if (!m_references)
            return;

        unref();
    }

    ALWAYS_INLINE T *data() { return m_data; }
    ALWAYS_INLINE const T *data() const { return m_data; }

    ALWAYS_INLINE size_t size() const { return m_size; }
    ALWAYS_INLINE size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    Result<void> append(const T& value)
    {
        TRY(copy_on_write());
        TRY(grow_if_needed());

        new (m_data + m_size) T(value);
        m_size += 1;

        return Result<void>();
    }

    Result<void> append(T&& value)
    {
        TRY(copy_on_write());
        TRY(grow_if_needed());

        new (m_data + m_size) T(value);
        m_size += 1;

        return Result<void>();
    }

    Result<void> append_iter(ForwardIterator<T> begin, ForwardIterator<T> end)
    {
        TRY(copy_on_write());

        size_t additonal_size = end - begin;
        TRY(ensure_at_least(additonal_size));

        size_t i = m_size;
        for (auto iter = begin; iter != end; iter++)
        {
            new (m_data + i) T(*iter);
            i += 1;
        }

        return Result<void>();
    }

    T& pop_front_unchecked()
    {
        T& value = m_data[m_size - 1];
        m_size -= 1;
        return value;
    }

    /**
     *  Clear the content of the vector and free the memory.
     */
    void clear()
    {
        if (!m_references)
        {
            return;
        }

        unref();
    }

    Result<void> clear_keep_capacity()
    {
        TRY(copy_on_write());

        for (size_t i = 0; i < m_size; i++)
            m_data[i].~T();
        m_size = 0;
    }

    Result<void> resize(size_t size)
    {
        if (m_capacity == 0)
        {
            TRY(grow_to(0, size));
            m_size = size;
        }

        if (size > m_size)
        {
            TRY(copy_on_write());

            if (size <= m_capacity)
            {
                m_size = m_capacity;
            }
            else
            {
                TRY(grow_to(m_capacity, size));
            }

            m_size = size;
        }
        else if (size < m_size)
        {
            TRY(copy_on_write());
            for (size_t i = size; i < m_size; i++)
                (m_data + i)->~T();
            m_size = size;

            // keep the capacity
        }

        return Result<void>();
    }

    Result<void> reserve(size_t capacity)
    {
        if (m_capacity >= capacity)
            return Result<void>();

        TRY(grow_to(m_capacity, capacity));
        return Result<void>();
    }

    void remove_at(size_t index)
    {
        if (index >= m_size)
            return;

        // TODO: do some testing.

        (m_data + index)->~T();
        memmove((void *)(m_data + index), (void *)(m_data + index + 1), sizeof(T) * m_size);
    }

    bool contains(const T& value) const
    {
        for (auto iter = begin(); iter != end(); iter++)
        {
            if (*iter == value)
                return true;
        }
        return false;
    }

    ForwardIterator<T> begin() const
    {
        return ForwardIterator<T>(data());
    }

    ForwardIterator<T> end() const
    {
        return ForwardIterator<T>(&data()[m_size]);
    }

    const T& get_unchecked(size_t index) const { return m_data[index]; }
    T& get_unchecked(size_t index) { return m_data[index]; }

    void operator=(const Vector& other)
    {
        if (m_references)
        {
            unref();
        }

        new (this) Vector<T>(other);
        // if (other.m_references)
        // {
        //     m_data = other.m_data;
        //     m_references = other.m_references;
        //     m_size = other.m_size;
        //     m_capacity = other.m_capacity;

        //     *m_references += 1;
        // }
    }

    static constexpr size_t initial_capacity = 3;
    static constexpr size_t growth_factor(size_t size)
    {
        return size * 2;
    }

private:
    T *m_data;
    uint32_t *m_references;
    size_t m_size;
    size_t m_capacity;

    Result<void> copy_on_write()
    {
        if (!m_references)
        {
            return Result<void>();
        }

        if (*m_references > 1)
        {
            *m_references -= 1;
            m_references = alloc<uint32_t>(1);
            if (!m_references)
                return Error(ErrorKind::OutOfMemory);

            T *new_data = alloc_array_uninitialized<T>(m_capacity);
            if (!new_data)
            {
                destroy(m_references);
                return Error(ErrorKind::OutOfMemory);
            }

            for (size_t i = 0; i < m_size; i++)
                new (new_data + i) T(m_data[i]);
            m_data = new_data;
        }

        return Result<void>();
    }

    Result<void> grow_if_needed()
    {
        if (m_capacity == 0)
        {
            m_references = alloc<uint32_t>(1);
            if (!m_references)
                return Error(ErrorKind::OutOfMemory);

            m_capacity = initial_capacity;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
            {
                destroy(m_references);
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
            m_references = alloc<uint32_t>(1);
            if (!m_references)
                return Error(ErrorKind::OutOfMemory);

            m_capacity = aditional_size;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
            {
                destroy(m_references);
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
        for (size_t i = 0; i < m_size; i++)
            new (new_data + i) T(m_data[i]);

        destroy_array_nodestruct(m_data, old_capacity);
        m_data = new_data;
        m_capacity = new_capacity;

        return Result<void>();
    }

    void unref()
    {
        *m_references -= 1;
        if (*m_references == 0)
        {
            destroy_array(m_data, m_size);
            destroy(m_references);

            m_data = nullptr;
            m_references = nullptr;
        }
        m_size = 0;
        m_capacity = 0;
    }
};
