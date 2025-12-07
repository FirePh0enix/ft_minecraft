#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Core/Alloc.hpp"
#include "Core/StringView.hpp"

/**
 * Capacity of a `String` before heap allocation is needed.
 */
constexpr size_t string_small_capacity = sizeof(char *) + sizeof(size_t) + sizeof(size_t) - 1;

class String
{
public:
    /**
     * Create an empty string.
     */
    String();

    String(const String& other);

    /**
     * Create a string from a NUL-terminated ascii sequence.
     */
    String(const char *str);

    /**
     * Create a string from an ascii sequence while specifying its size.
     */
    String(const char *str, size_t size);

    String(const StringView& sv)
        : String(sv.data(), sv.size())
    {
    }

    String& operator=(const String& other)
    {
        if (!is_small() && large.ptr)
            destroy_n(large.ptr);

        // FIXME: This is a hack that I despise.
        large.capacity = 0;
        large.size = 0;
        large.ptr = nullptr;
        small.small_flag = 1;

        append(other.data(), other.size());
        return *this;
    }

    String& operator=(const char *str)
    {
        if (!is_small() && large.ptr)
            destroy_n(large.ptr);

        *this = String(str);
        return *this;
    }

    void operator+=(const char *str) { append(str, strlen(str)); }
    void operator+=(const String& str) { append(str); }
    void operator+=(char c) { append(c); }

    std::strong_ordering operator<=>(const String& other) const
    {
        return compare(data(), size(), other.data(), other.size());
    }

    bool operator==(const String& other) const
    {
        return (*this <=> other) == std::strong_ordering::equal;
    }

    constexpr bool is_small() const { return small.small_flag; }

    size_t size() const { return is_small() ? small.size : large.size; }
    size_t capacity() const { return is_small() ? string_small_capacity : large.capacity; }

    const char *data() const { return is_small() ? small.data : large.ptr; }
    char *data() { return is_small() ? small.data : large.ptr; }

    void append(const char *str, size_t size);
    void append(const String& str) { append(str.data(), str.size()); }
    void append(char c) { append(&c, 1); }

    bool contains(char c) const { return StringView(*this).contains(c); }
    bool starts_with(const StringView& prefix) const { return StringView(*this).starts_with(prefix); }
    StringView slice(size_t offset, size_t length) const { return StringView(*this).slice(offset, length); }
    StringView slice(size_t offset) const { return StringView(*this).slice(offset); }

protected:
    // void replace_by(const String& str)
    // {
    //     if (!is_small() && large.ptr)
    //     {
    //         destroy_n(large.ptr);
    //     }

    //     if (str.is_small())
    //     {
    //         small.small_flag = 1;
    //         small.size = str.small.size;
    //         memcpy(small.data, str.small.data, small.size + 1);
    //     }
    //     else
    //     {
    //         small.small_flag = 0;
    //         large.ptr = alloc_n<char>(str.large.capacity);
    //         memcpy(large.ptr, str.large.ptr, str.large.size + 1);
    //         large.size = str.large.size;
    //         large.capacity = str.large.capacity;
    //     }
    // }

    static std::strong_ordering compare(const char *str1, size_t size1, const char *str2, size_t size2)
    {
        // TODO: Add our own constexpr implementation.

        (void)size1;
        (void)size2;
        int value = std::strcmp(str1, str2);
        if (value == 0)
            return std::strong_ordering::equal;
        else if (value < 0)
            return std::strong_ordering::less;
        else
            return std::strong_ordering::greater;
    }

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

    union
    {
        struct
        {
            size_t size = 0;
            size_t capacity = 0;
            char *ptr = nullptr;
        } large;
        struct
        {
            char data[string_small_capacity] = {};
            uint8_t size : 7 = 0;
            uint8_t small_flag : 1 = 1;
        } small;
    };
};

inline std::ostream& operator<<(std::ostream& os, const String& s)
{
    os << s.data();
    return os;
}
