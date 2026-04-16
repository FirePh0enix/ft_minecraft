#pragma once

#include <cstring>
#include <filesystem>
#include <string>

#include "Core/Definitions.hpp"

class String;

class StringView
{
public:
    ALWAYS_INLINE constexpr StringView(const char *str)
        : m_data(str), m_size(strlen(str))
    {
    }

    ALWAYS_INLINE constexpr StringView(const char *str, size_t size)
        : m_data(str), m_size(size)
    {
    }

    ALWAYS_INLINE StringView(const std::filesystem::path& path)
        : m_data(path.c_str()), m_size(std::strlen(path.c_str()))
    {
    }

    StringView(const String& string);

    const char& operator[](size_t index) const
    {
        return data()[index];
    }

    std::strong_ordering operator<=>(const StringView& other) const
    {
        int value = std::memcmp(m_data, other.m_data, m_size);
        return value == 0 ? std::strong_ordering::equal : (value < 0 ? std::strong_ordering::less : std::strong_ordering::greater);
    }

    bool operator==(const StringView& other) const { return *this <=> other == std::strong_ordering::equal; }

    ALWAYS_INLINE const char *data() const
    {
        return m_data;
    }

    constexpr ALWAYS_INLINE size_t size() const
    {
        return m_size;
    }

    bool contains(char c) const;
    bool starts_with(const StringView& prefix) const;

    StringView slice(size_t offset, size_t length) const;
    StringView slice(size_t offset) const { return slice(offset, m_size - offset); }

private:
    const char *m_data;
    size_t m_size;

    static constexpr size_t strlen(const char *str)
    {
        size_t len = 0;
        while (str[len])
            len++;
        return len;
    }

    static constexpr void memcpy(char *dest, const char *source, size_t len)
    {
        for (size_t i = 0; i < len; i++)
            dest[i] = source[i];
    }

    static constexpr void memset(char *dest, char c, size_t len)
    {
        for (size_t i = 0; i < len; i++)
            dest[i] = c;
    }
};

inline std::ostream& operator<<(std::ostream& os, const StringView& sv)
{
    os << sv.data();
    return os;
}
