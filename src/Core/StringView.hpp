#pragma once

#include <cstring>

#include "Core/Definitions.hpp"

class StringView
{
public:
    ALWAYS_INLINE StringView(const char *str)
        : m_data(str), m_size(std::strlen(str))
    {
    }

    ALWAYS_INLINE StringView(const char *str, size_t size)
        : m_data(str), m_size(size)
    {
    }

    ALWAYS_INLINE StringView(const std::string& str)
        : m_data(str.data()), m_size(str.size())
    {
    }

    operator std::string() const
    {
        std::string s;
        s.append(m_data, m_size);
        return s;
    }

    ALWAYS_INLINE const char *c_str() const
    {
        return m_data;
    }

    ALWAYS_INLINE size_t size() const
    {
        return m_size;
    }

private:
    const char *m_data;
    size_t m_size;
};

inline std::ostream& operator<<(std::ostream& os, const StringView& sv)
{
    os << sv.c_str();
    return os;
}
