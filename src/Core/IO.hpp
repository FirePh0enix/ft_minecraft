#pragma once

#include "Core/Containers/LocalVector.hpp"
#include "Core/Result.hpp"
#include "Variant.hpp"

#include <cstddef>

class Reader
{
public:
    virtual Result<size_t> read_raw(void *buf, size_t size) = 0;
    virtual size_t size() = 0;
    virtual bool eof() = 0;

    Result<String> read_to_string();
    Result<void> read_to_buffer(LocalVector<char>& buf);
    Result<Variant> read_variant();
};

class Writer
{
public:
    virtual Result<size_t> write_raw(const void *buf, size_t size) = 0;

    Result<void> write_variant(const Variant& variant);
};
