#pragma once

/**
 * @brief A `std::vector` stored on the stack backed by an `std::array`.
 */
template <typename T, const size_t capacity>
class StackVector
{
public:
    StackVector()
        : m_size(0)
    {
    }

    StackVector(const std::initializer_list<T>& list)
        : m_data(list), m_size(list.size())
    {
    }

    void push_back(const T& value)
    {
        m_data[m_size] = value;
        m_size += 1;
    }

    inline const T *data() const
    {
        return m_data;
    }

    inline T *data()
    {
        return m_data.data();
    }

    inline size_t size() const
    {
        return m_size;
    }

private:
    std::array<T, capacity> m_data;
    size_t m_size;
};
