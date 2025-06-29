#pragma once

#include "Core/Assert.hpp"
#include "Core/Containers/Iterator.hpp"

/**
 * @brief A container that provide a similar interface than `std::vector`, except that the data is stored on the stack.
 */
template <typename T, const size_t capacity>
class StackVector
{
public:
    StackVector()
        : m_size(0)
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

    inline size_t size() const
    {
        return m_size;
    }

    inline const T *data() const
    {
        return m_data.data();
    }

    inline T *data()
    {
        return m_data.data();
    }

    void push_back(T&& value)
    {
        ASSERT(m_size < capacity, "StackVector capacity exceeded");

        m_data[m_size] = value;
        m_size++;
    }

    void push_back(const T& value)
    {
        ASSERT(m_size < capacity, "StackVector capacity exceeded");

        m_data[m_size] = value;
        m_size++;
    }

    ForwardIterator<T> begin()
    {
        return ForwardIterator<T>(data());
    }

    ForwardIterator<T> end()
    {
        return ForwardIterator<T>(data() + m_size);
    }

private:
    std::array<T, capacity> m_data;
    size_t m_size;
};
