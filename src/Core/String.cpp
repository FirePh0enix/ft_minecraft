#include "Core/String.hpp"
#include "Core/Print.hpp"

String::String()
{
    small.small_flag = 1;
    small.size = 0;
    memset(small.data, 0, string_small_capacity);
}

String::String(const String& other)
{
    if (other.is_small())
    {
        small.small_flag = 1;
        small.size = other.small.size;
        memcpy(small.data, other.small.data, small.size + 1);
    }
    else
    {
        small.small_flag = 1;
        large.capacity = other.large.capacity;
        large.size = other.large.size;
        large.ptr = alloc_n<char>(large.capacity);
        memcpy(large.ptr, other.large.ptr, large.size + 1);
    }
}

String::String(const char *str)
    : String(str, strlen(str))
{
}

String::String(const char *str, size_t size)
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
        small.small_flag = 1;
        small.size = 0;
        append(str, size);
    }
}

void String::resize(size_t new_size)
{
    char *new_ptr = alloc_n<char>(new_size + 1);
    std::memcpy(new_ptr, data(), size() + 1);

    if (is_small())
    {
        small.small_flag = 0;
        large.ptr = new_ptr;
        large.capacity = new_size + 1;
        large.size = new_size;
    }
    else
    {
        destroy_n(large.ptr);
        large.ptr = new_ptr;
        large.capacity = new_size + 1;
        large.size = new_size;
    }
}

void String::append(const char *str, size_t size)
{
    if (is_small())
    {
        // Check if the new string fit in the small string.
        const size_t new_len = size + small.size;

        if (new_len + 1 <= string_small_capacity)
        {
            memcpy(small.data + small.size, str, size);
            small.data[new_len] = '\0';
        }
        else
        {
            char *new_ptr = alloc_n<char>(new_len + 1);
            memcpy(new_ptr, small.data, small.size);
            memcpy(new_ptr + small.size, str, size);
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

        if (new_size + 1 >= large.capacity)
        {
            large.capacity = new_size + 1;

            char *new_ptr = alloc_n<char>(new_size + 1);
            memcpy(new_ptr, large.ptr, large.size);
            new_ptr[large.size] = '\0';

            destroy_n(large.ptr);
            large.ptr = new_ptr;
        }

        memcpy(large.ptr + large.size, str, size);
        large.ptr[new_size] = '0';
        large.size = new_size;
    }
}
