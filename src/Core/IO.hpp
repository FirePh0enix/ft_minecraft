#pragma once

#include "Core/Result.hpp"
#include "Variant.hpp"

#include <cstddef>

class Reader
{
public:
    virtual Result<size_t> read_raw(void *buf, size_t size) = 0;
    virtual size_t size() = 0;
    virtual bool eof() = 0;

    Result<std::string> read_to_string();
    Result<void> read_to_buffer(std::vector<char>& buf);
    Result<std::optional<Variant>> read_variant();
};

class Writer
{
public:
    virtual Result<size_t> write_raw(const void *buf, size_t size) = 0;

    Result<void> write_variant(const Variant& variant);
};

class BufferReader : public Reader
{
public:
    BufferReader(const uint8_t *buffer, size_t size) : m_buffer(buffer), m_size(size) {}

    virtual Result<size_t> read_raw(void *buf, size_t size) override;
    virtual size_t size() override;
    virtual bool eof() override;

private:
    const uint8_t *m_buffer;
    size_t m_size;
    size_t m_cursor = 0;
};

/**
 * Writer backed by a dynamic array.
 */
class BufferWriter : public Writer
{
public:
    virtual Result<size_t> write_raw(const void *buf, size_t size) override;

    std::span<const uint8_t> buffer() const { return m_buffer; }

private:
    std::vector<uint8_t> m_buffer;
};
