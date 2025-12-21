#pragma once

#include "Core/Definitions.hpp"

#include <cstring>
#include <vector>

class DataBuffer
{
public:
    DataBuffer(size_t capacity)
    {
        m_data.reserve(capacity);
    }

    ALWAYS_INLINE size_t size() const
    {
        return m_data.size();
    }

    ALWAYS_INLINE const char *data() const
    {
        return m_data.data();
    }

    void clear_keep_capacity()
    {
        m_data.resize(0);
    }

    template <typename T>
    void add(const T& value)
    {
        constexpr size_t size = sizeof(T);
        const size_t previous_vec_size = m_data.size();

        m_data.resize(previous_vec_size + sizeof(T));
        std::memcpy(m_data.data() + previous_vec_size, (char *)&value, size);
    }

private:
    std::vector<char> m_data;
};
