#pragma once

#include "Core/Assert.hpp"
#include "Core/Containers/Iterator.hpp"
#include "Core/Format.hpp"

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
        ASSERT_V(index < m_size, "Index {} out of bounds 0..{}", index, m_size);
        return m_data[index];
    }

    T& operator[](size_t index)
    {
        ASSERT_V(index < m_size, "Index out of bounds 0..{}", index, m_size);
        return m_data[index];
    }

    bool operator==(const StackVector& other) const
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

template <typename T, const size_t capacity>
struct Formatter<StackVector<T, capacity>>
{
    void format(const StackVector<T, capacity>& vec, FormatContext& ctx) const
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
