#pragma once

#include "Core/Math.hpp"
#include "Core/String.hpp"
#include "glm/ext/quaternion_float.hpp"

#include <nlohmann/json.hpp>

#include <variant>

enum class VariantType
{
    Bool,
    String,
    Vec2,
    Vec3,
    Quat,
};
using Variant = std::variant<std::monostate, bool, String, glm::vec2, glm::vec3, glm::quat>;

template <typename T>
inline VariantType get();

template <>
inline VariantType get<bool>() { return VariantType::Bool; }

template <>
inline VariantType get<String>() { return VariantType::Bool; }

inline void from_json(const nlohmann::json& j, glm::vec2& m)
{
    std::array<float, 2> array = j;
    m = glm::vec2(array[0], array[1]);
}

inline void from_json(const nlohmann::json& j, glm::vec3& m)
{
    std::array<float, 3> array = j;
    m = glm::vec3(array[0], array[1], array[2]);
}

inline void from_json(const nlohmann::json& j, glm::quat& m)
{
    std::array<float, 4> array = j;
    m = glm::quat(array[0], array[1], array[2], array[3]);
}

inline void from_json(const nlohmann::json& j, Variant& m)
{
    m = j;
}

inline void to_json(nlohmann::json& j, const glm::vec2& m)
{
    std::array<float, 2> array{m.x, m.y};
    j = array;
}

inline void to_json(nlohmann::json& j, const glm::vec3& m)
{
    std::array<float, 3> array{m.x, m.y, m.z};
    j = array;
}

inline void to_json(nlohmann::json& j, const glm::quat& m)
{
    std::array<float, 4> array{m.w, m.x, m.y, m.z};
    j = array;
}

inline void to_json(nlohmann::json& j, const Variant& m)
{
    j = m;
}
