#pragma once

#include <bitset>
#include <unordered_map>

namespace stdext
{

struct string_hash
{
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const char *str) const { return hash_type{}(str); }
    size_t operator()(std::string_view str) const { return hash_type{}(str); }
    size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

template <typename T>
using string_map = std::unordered_map<std::string, T, string_hash, std::equal_to<>>;

} // namespace stdext
