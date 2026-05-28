#pragma once

#include "Core/Containers/Array.hpp"
#include "Core/Containers/Vector.hpp"

#include <nlohmann/json.hpp>

/**
std::map, std::vector, std::basic_string<char>, bool, long, unsigned long, double, std::allocator, nlohmann::adl_serializer, std::vector<unsigned char>, void
*/

// using Json = nlohmann::basic_json<HashMap, Vector, bool, int64_t, uint64_t, double, std::allocator<void>, nlohmann::adl_serializer, Vector<unsigned char>, void>;

// TODO: Under the hood, json.hpp still uses the STL, but it may be possible to personalize this,
//       or change to another json library.

template <typename T>
inline void from_json(const nlohmann::json& j, Vector<T>& m)
{
    const auto s = j.get_ptr<const nlohmann::json::array_t *>();

    EXPECT(m.reserve(s->size()));
    for (size_t i = 0; i < s->size(); i++)
        EXPECT(m.append(s->at(i)));
}

template <typename T>
inline void to_json(nlohmann::json& j, const Vector<T>& m)
{
    const auto s = j.get_ptr<nlohmann::json::array_t *>();
    for (size_t i = 0; i < m.size(); i++)
        s->emplace_back(m.get_unchecked(i));
}

template <typename T, const size_t _S>
inline void from_json(const nlohmann::json& j, Array<T, _S>& m)
{
    const auto s = j.get_ptr<const nlohmann::json::array_t *>();
    for (size_t i = 0; i < s->size(); i++)
        m[i] = s->at(i);
}

template <typename T, const size_t _S>
inline void to_json(nlohmann::json& j, const Array<T, _S>& m)
{
    const auto s = j.get_ptr<nlohmann::json::array_t *>();
    for (size_t i = 0; i < _S; i++)
        s->emplace_back(m[i]);
}

inline void from_json(const nlohmann::json& j, String& m)
{
    const auto s = j.get_ptr<const nlohmann::json::string_t *>();
    m = String(s->data(), s->size());
}

inline void to_json(nlohmann::json& j, const String& m)
{
    const auto s = j.get_ptr<nlohmann::json::string_t *>();
    s->append(m.data(), m.size());
}
