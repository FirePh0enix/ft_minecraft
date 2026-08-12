#pragma once

#include "Core/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

class ZLib
{
public:
    static Result<void> deflate(std::span<const std::byte> data, std::vector<uint8_t>& compressed_data);
    static Result<void> inflate(std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data);
};
