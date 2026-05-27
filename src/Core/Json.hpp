#pragma once

#include "Core/Containers/Vector.hpp"

#include <nlohmann/json.hpp>

/**
std::map, std::vector, std::basic_string<char>, bool, long, unsigned long, double, std::allocator, nlohmann::adl_serializer, std::vector<unsigned char>, void
*/

// using Json = nlohmann::basic_json<HashMap, Vector, bool, int64_t, uint64_t, double, std::allocator<void>, nlohmann::adl_serializer, Vector<unsigned char>, void>;

template <typename T>
inline void from_json(const nlohmann::json& j, Vector<T>& m)
{
    const auto s = j.get_ptr<const nlohmann::json::array_t *>();

    EXPECT(m.reserve(s->size()));
    for (size_t i = 0; i < s->size(); i++)
        EXPECT(m.append((*s)[i]));
}

void from_json(const nlohmann::json& j, String& m)
{
    const auto s = j.get_ptr<const nlohmann::json::string_t *>();
    m = String(s->data(), s->size());
}
