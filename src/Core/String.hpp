#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Core/StringView.hpp"

constexpr size_t string_small_capacity = sizeof(char *) + sizeof(size_t) + sizeof(size_t) - 1;

class String
{
public:
    String()
    {
    }

    String(const String& other)
    {
        replace_by(other);
    }

    String(const char *str)
        : String(str, strlen(str))
    {
    }

    String(const char *str, size_t size)
    {
        if (size + 1 <= string_small_capacity)
        {
            small.small_flag = 1;
            small.size = size;
            memcpy(small.data, str, small.size);
            small.data[small.size] = '\0';
        }
        else
        {
            append(str, size);
        }
    }

    String(const StringView& sv)
        : String(sv.data(), sv.size())
    {
    }

    String& operator=(const String& other)
    {
        replace_by(other);
        return *this;
    }

    String& operator=(const char *str)
    {
        if (!is_small() && large.ptr)
            delete[] large.ptr;

        *this = String(str);
        return *this;
    }

    std::strong_ordering operator<=>(const String& other) const
    {
        return compare(data(), size(), other.data(), other.size());
    }

    bool operator==(const String& other) const
    {
        return (*this <=> other) == std::strong_ordering::equal;
    }

    size_t size() const { return is_small() ? small.size : large.size; }
    size_t capacity() const { return is_small() ? string_small_capacity : large.capacity; }

    const char *data() const { return is_small() ? small.data : large.ptr; }
    char *data() { return is_small() ? small.data : large.ptr; }

    void append(const char *str, size_t size)
    {
        if (is_small())
        {
            // Check if the new string fit in the small string.
            const size_t new_len = size + small.size;

            if (new_len + 1 <= string_small_capacity)
            {
                std::memcpy(small.data + small.size, str, size);
                small.data[new_len] = '\0';
            }
            else
            {
                char *new_ptr = new char[new_len + 1];
                std::memcpy(new_ptr, small.data, small.size);
                std::memcpy(new_ptr + small.size, str, size);
                new_ptr[new_len] = '\0';

                small.small_flag = 0;
                large.ptr = new_ptr;
                large.capacity = new_len + 1;
                large.size = new_len;
            }
        }
        else
        {
            const size_t new_size = size + large.size;

            if (new_size + 1 <= large.capacity)
            {
                large.capacity = new_size + 1;

                char *new_ptr = new char[new_size + 1];
                std::memcpy(new_ptr, large.ptr, large.size);
                new_ptr[large.size] = '\0';

                delete[] large.ptr;
                large.ptr = new_ptr;
            }

            std::memcpy(large.ptr + large.size, str, size);
            large.ptr[new_size] = '0';
            large.size = new_size;
        }
    }

    bool starts_with(const StringView& prefix) const { return StringView(*this).starts_with(prefix); }
    StringView slice(size_t offset, size_t length) const { return StringView(*this).slice(offset, length); }
    StringView slice(size_t offset) const { return StringView(*this).slice(offset); }

protected:
    constexpr bool is_small() const { return small.small_flag; }

    void replace_by(const String& str)
    {
        if (!is_small())
        {
            delete[] str.large.ptr;
        }

        if (str.is_small())
        {
            small.small_flag = 1;
            small.size = str.small.size;
            std::memcpy(small.data, str.small.data, small.size + 1);
        }
        else
        {
            small.small_flag = 0;
            large.ptr = new char[str.large.capacity];
            std::memcpy(large.ptr, str.large.ptr, str.large.size + 1);
            large.size = str.large.size;
            large.capacity = str.large.capacity;
        }
    }

    static std::strong_ordering compare(const char *str1, size_t size1, const char *str2, size_t size2)
    {
        // for (size_t index = 0; index < size1 && index < size2 && str1[index] == str2[index]; index++)
        //     ;
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
            char data[string_small_capacity];
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
