#include "Core/String.hpp"
#include "Core/Alloc.hpp"

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
        small.small_flag = 0;
        large.capacity = other.large.capacity;
        large.size = other.large.size;
        large.ptr = alloc_array_uninitialized<char>(large.capacity);
        memcpy(large.ptr, other.large.ptr, large.size + 1);
    }
}

String::String(const char *str)
    : String(str, strlen(str))
{
}

String::String(const char *str, size_t size)
    : String()
{
    append(str, size);
}

String::~String()
{
    if (!is_small())
        destroy_array_nodestruct(large.ptr, large.capacity);
}

void String::resize(size_t new_size)
{
    // TODO: if `new_size` < string_small_capacity then stay/go back to a small string.

    char *new_ptr = alloc_array_uninitialized<char>(new_size + 1);
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
        destroy_array(large.ptr, large.size);
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
            small.size = new_len;
        }
        else
        {
            char *new_ptr = alloc_array_uninitialized<char>(new_len + 1);
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

            char *new_ptr = alloc_array_uninitialized<char>(new_size + 1);
            memcpy(new_ptr, large.ptr, large.size);
            new_ptr[large.size] = '\0';

            destroy_array(large.ptr, 0);
            large.ptr = new_ptr;
        }

        memcpy(large.ptr + large.size, str, size);
        large.ptr[new_size] = '0';
        large.size = new_size;
    }
}
