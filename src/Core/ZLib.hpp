#pragma once

#include "Core/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <stop_token>
#include <vector>

class ZLib
{
public:
    static Result<void> deflate(std::span<const std::byte> data, std::vector<uint8_t>& compressed_data);
    static Result<void> deflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& compressed_data);
    static Result<void> inflate(std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data);
    static Result<void> inflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data);
};
