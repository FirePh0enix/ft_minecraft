#pragma once

#include <cstring>
#include <filesystem>
#include <string>

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

    ALWAYS_INLINE StringView(const std::filesystem::path& path)
        : m_data(path.c_str()), m_size(std::strlen(path.c_str()))
    {
    }

    operator std::string() const
    {
        std::string s;
        s.append(m_data, m_size);
        return s;
    }

    std::strong_ordering operator<=>(const StringView& other) const
    {
        int value = std::memcmp(m_data, other.m_data, m_size);
        return value == 0 ? std::strong_ordering::equal : (value < 0 ? std::strong_ordering::less : std::strong_ordering::greater);
    }

    bool operator==(const StringView& other) const = default;
    bool operator!=(const StringView& other) const = default;
    bool operator<=(const StringView& other) const = default;
    bool operator>=(const StringView& other) const = default;
    bool operator<(const StringView& other) const = default;
    bool operator>(const StringView& other) const = default;

    ALWAYS_INLINE const char *c_str() const
    {
        return m_data;
    }

    ALWAYS_INLINE size_t size() const
    {
        return m_size;
    }

    StringView slice(size_t offset, size_t length) const;

    StringView slice(size_t offset) const
    {
        return slice(offset, m_size - offset);
    }

    bool starts_with(const StringView& prefix) const;

private:
    const char *m_data;
    size_t m_size;
};

inline std::ostream& operator<<(std::ostream& os, const StringView& sv)
{
    os << sv.c_str();
    return os;
}
