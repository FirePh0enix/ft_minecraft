#pragma once

#include "Core/Alloc.hpp"
#include "Core/Assert.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Definitions.hpp"
#include "Core/Option.hpp"

#include <cstddef>
#include <functional>

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
    static Vector create(Args&&...args)
    {
        Vector v;
        v.reserve(sizeof...(args));
        (v.append(args), ...);
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

    const T& operator[](size_t index) const
    {
        ASSERT_V(index < m_size, "index out of bounds");
        return get_unchecked(index);
    }
    T& operator[](size_t index)
    {
        ASSERT_V(index < m_size, "index out of bounds");
        return get_unchecked(index);
    }

    ALWAYS_INLINE T *data() { return m_data; }
    ALWAYS_INLINE const T *data() const { return m_data; }

    ALWAYS_INLINE size_t size() const { return m_size; }
    ALWAYS_INLINE size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    void append(const T& value)
    {
        copy_on_write();
        grow_if_needed();

        new (m_data + m_size) T(value);
        m_size += 1;
    }

    void append(T&& value)
    {
        copy_on_write();
        grow_if_needed();

        new (m_data + m_size) T(value);
        m_size += 1;
    }

    void append_iter(ForwardIterator<T> begin, ForwardIterator<T> end)
    {
        copy_on_write();

        size_t additonal_size = end - begin;
        ensure_at_least(additonal_size);

        size_t i = m_size;
        for (auto iter = begin; iter != end; iter++)
        {
            new (m_data + i) T(*iter);
            i += 1;
        }
    }

    T pop_one_unchecked()
    {
        copy_on_write();

        T value = m_data[m_size - 1];
        m_size -= 1;
        m_data[m_size].~T();
        return value;
    }

    void remove_one()
    {
        copy_on_write();

        ASSERT(m_size >= 1, "");
        m_size -= 1;
        m_data[m_size].~T();
    }

    /**
     * Clear the content of the vector and free the memory.
     */
    void clear()
    {
        if (!m_references)
        {
            return;
        }

        unref();
    }

    void clear_keep_capacity()
    {
        copy_on_write();

        for (size_t i = 0; i < m_size; i++)
            m_data[i].~T();
        m_size = 0;
    }

    void resize(size_t size)
    {
        if (m_capacity == 0)
        {
            grow_to(0, size);
            for (size_t i = m_size; i < size; i++)
                new (&m_data[i]) T();
            m_size = size;
        }

        if (size > m_size)
        {
            copy_on_write();

            if (size <= m_capacity)
            {
                m_size = m_capacity;
            }
            else
            {
                grow_to(m_capacity, size);
            }

            for (size_t i = m_size; i < size; i++)
                new (&m_data[i]) T();

            m_size = size;
        }
        else if (size < m_size)
        {
            copy_on_write();
            for (size_t i = size; i < m_size; i++)
                (m_data + i)->~T();
            m_size = size;

            // keep the capacity
        }
    }

    void reserve(size_t capacity)
    {
        if (m_capacity >= capacity)
            return;

        copy_on_write();
        grow_to(m_capacity, capacity);
    }

    void remove_at(size_t index)
    {
        if (index >= m_size)
            return;

        copy_on_write();

        // TODO: do some testing.

        (m_data + index)->~T();
        m_size -= 1;
        memmove((void *)(m_data + index), (void *)(m_data + index + 1), sizeof(T) * (m_size - index));
    }

    void remove_if(std::function<bool(const T&)> f)
    {
        copy_on_write();

        for (size_t i = 0; i < m_size; i++)
        {
            if (f(m_data[i]))
            {
                remove_at(i);
                return;
            }
        }
    }

    Option<size_t> index_of(const T& value)
    {
        for (size_t i = 0; i < m_size; i++)
            if (m_data[i] == value)
                return i;
        return None;
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

    ALWAYS_INLINE const T& get_unchecked(size_t index) const { return m_data[index]; }
    ALWAYS_INLINE T& get_unchecked(size_t index) { return m_data[index]; }

    uint32_t references() const { return m_references ? *m_references : 0; }

    void operator=(const Vector& other)
    {
        if (m_references)
        {
            unref();
        }

        new (this) Vector<T>(other);
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

    void copy_on_write()
    {
        if (!m_references)
        {
            return;
        }

        if (*m_references > 1)
        {
            *m_references -= 1;
            m_references = alloc<uint32_t>(1);
            T *new_data = alloc_array_uninitialized<T>(m_capacity);

            for (size_t i = 0; i < m_size; i++)
                new (new_data + i) T(m_data[i]);
            m_data = new_data;
        }
    }

    void grow_if_needed()
    {
        if (m_capacity == 0)
        {
            m_references = alloc<uint32_t>(1);
            m_capacity = initial_capacity;
            m_data = alloc_array_uninitialized<T>(m_capacity);
        }
        else if (m_size == m_capacity)
        {
            size_t new_capacity = growth_factor(m_capacity);
            grow_to(m_capacity, new_capacity);
        }
    }

    void ensure_at_least(size_t aditional_size)
    {
        if (m_capacity == 0)
        {
            m_references = alloc<uint32_t>(1);
            m_capacity = aditional_size;
            m_data = alloc_array_uninitialized<T>(m_capacity);
            if (!m_data)
            {
                destroy(m_references);
            }
        }
        else if (m_size == m_capacity)
        {
            size_t new_capacity = m_size + aditional_size;
            grow_to(m_capacity, new_capacity);
        }
    }

    void grow_to(size_t old_capacity, size_t new_capacity)
    {
        (void)old_capacity;
        T *new_data = alloc_array_uninitialized<T>(new_capacity);

        // perform a deep copy.
        // TODO: uncessary, elements are moved not copied. memcpy should be enough.
        for (size_t i = 0; i < m_size; i++)
            new (new_data + i) T(m_data[i]);

        destroy_array(m_data, m_size);
        m_data = new_data;
        m_capacity = new_capacity;

        if (!m_references)
            m_references = alloc<uint32_t>(1);
    }

    void unref()
    {
        *m_references -= 1;
        if (*m_references == 0)
        {
            destroy_array(m_data, m_size);
            destroy(m_references);
        }
        m_size = 0;
        m_capacity = 0;
        m_data = nullptr;
        m_references = nullptr;
    }
};
