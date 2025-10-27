#pragma once

#include <array>

#include "Core/Assert.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Format.hpp"

/**
 * @brief A container that provide a similar interface than `std::vector`, except that the data is stored on the stack.
 */
template <typename T, const size_t capacity>
class InplaceVector
{
public:
    constexpr InplaceVector()
        : m_size(0)
    {
    }

    constexpr InplaceVector(const std::initializer_list<T>& list)
        : m_size(list.size())
    {
        for (size_t i = 0; i < m_size; i++)
            m_data[i] = *(list.begin() + i);
    }

    template <const size_t other_capacity>
    constexpr InplaceVector(const InplaceVector<T, other_capacity>& vec, const std::initializer_list<T>& list)
        : m_size(vec.size() + list.size())
    {
        size_t i = 0;
        for (; i < vec.size(); i++)
            m_data[i] = vec[i];

        for (size_t j = 0; j < list.size(); i++, j++)
            m_data[i] = *(list.begin() + j);
    }

    const T& operator[](size_t index) const
    {
        ASSERT_V(index < m_size, "Index {} out of bounds 0..{}", index, m_size);
        return m_data[index];
    }

    T& operator[](size_t index)
    {
        ASSERT_V(index < m_size, "Index out of bounds 0..{}", index, m_size);
        return m_data[index];
    }

    bool operator==(const InplaceVector& other) const
    {
        if (m_size != other.m_size)
            return false;

        for (size_t i = 0; i < m_size; i++)
        {
            if (m_data[i] != other.m_data[i])
                return false;
        }

        return true;
    }

    constexpr size_t size() const
    {
        return m_size;
    }

    constexpr size_t max_capacity() const
    {
        return capacity;
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

template <typename T, const size_t capacity>
struct Formatter<InplaceVector<T, capacity>>
{
    void format(const InplaceVector<T, capacity>& vec, FormatContext& ctx) const
    {
        ctx.write_str("{ ");

        for (size_t i = 0; i < vec.size(); i++)
        {
            format_to(ctx.out(), "{}", vec[i]);

            if (i + 1 < vec.size())
                ctx.write_str(", ");
        }

        ctx.write_str(" }");
    }
};
