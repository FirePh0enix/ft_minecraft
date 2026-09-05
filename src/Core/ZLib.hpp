#pragma once

#include "Core/Error.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <stop_token>
#include <vector>

class ZLib
{
public:
    static std::expected<void, Error> deflate(std::span<const std::byte> data, std::vector<uint8_t>& compressed_data);
    static std::expected<void, Error> deflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& compressed_data);
    static std::expected<void, Error> inflate(std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data);
    static std::expected<void, Error> inflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data);
};
