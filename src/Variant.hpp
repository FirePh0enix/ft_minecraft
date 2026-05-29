#pragma once

#include "Core/Containers/Array.hpp"
#include "Core/Json.hpp"
#include "Core/Math.hpp"
#include "Core/String.hpp"
#include "Item/ItemStack.hpp"

#include <nlohmann/json.hpp>

enum class VariantType : uint32_t
{
    Null,
    /**
     * Boolean primitive, either true or false.
     */
    Bool,
    /**
     * 64-bits precision floating point number.
     */
    Double,
    /**
     * 64-bits signed integer.
     */
    Integer,
    /**
     * String of character.
     */
    String,
    /**
     * 2 dimensional vector.
     */
    Vec2,
    /**
     * 3 dimensional vector.
     */
    Vec3,
    /**
     * Quaternion.
     */
    Quat,
    /**
     * Stack of item.
     */
    ItemStack,
};

struct __attribute__((aligned(16))) Variant
{
    uint8_t data[24]{0};
    VariantType tag;

    Variant() : tag(VariantType::Null) {}
    Variant(std::nullptr_t) : tag(VariantType::Null) {}

    Variant(bool b) : tag(VariantType::Bool) { data[0] = (uint8_t)b; }
    Variant(double d) : tag(VariantType::Double) { *((double *)data) = d; }
    Variant(int64_t i) : tag(VariantType::Integer) { *((int64_t *)data) = i; }
    Variant(const String& s) : tag(VariantType::String) { new ((String *)data) String(s); }
    Variant(glm::vec2 v) : tag(VariantType::Vec2) { *((glm::vec2 *)data) = v; }
    Variant(glm::vec3 v) : tag(VariantType::Vec3) { *((glm::vec3 *)data) = v; }
    Variant(glm::quat q) : tag(VariantType::Quat) { *((glm::quat *)data) = q; }
    Variant(ItemStack is) : tag(VariantType::ItemStack) { *((ItemStack *)data) = is; }

    Variant(const Variant& v)
        : tag(v.tag)
    {
        if (v.has(VariantType::String))
        {
            const String& s = v.get_unchecked<String>();
            new (data) String(s);
        }
        else
        {
            memcpy(data, v.data, 24);
        }
    }

    ~Variant()
    {
        if (has(VariantType::String))
            ((String *)data)->~String();
    }

    Variant& operator=(const Variant& v)
    {
        this->~Variant();
        new (this) Variant(v);
        return *this;
    }

    constexpr bool has(VariantType expected) const { return tag == expected; }

    template <typename T>
    const T& get_unchecked() const
    {
        return *(const T *)data;
    }
};

inline void from_json(const nlohmann::json& j, glm::vec2& m)
{
    Array<float, 2> array = j;
    m = glm::vec2(array[0], array[1]);
}

inline void from_json(const nlohmann::json& j, glm::vec3& m)
{
    Array<float, 3> array = j;
    m = glm::vec3(array[0], array[1], array[2]);
}

inline void from_json(const nlohmann::json& j, glm::quat& m)
{
    Array<float, 4> array = j;
    m = glm::quat(array[0], array[1], array[2], array[3]);
}

inline void from_json(const nlohmann::json& j, Variant& m)
{
    if (j.is_boolean())
    {
        m = bool(j);
    }
    else if (j.is_number_float())
    {
        m = double(j);
    }
    else if (j.is_number_integer())
    {
        m = int64_t(j);
    }
    else if (j.is_string())
    {
        String s = j;
        m = s;
    }
    else if (j.is_array() && j.size() == 2)
    {
        Array<float, 2> a = j;
        m = glm::vec2(a[0], a[1]);
    }
    else if (j.is_array() && j.size() == 3)
    {
        Array<float, 3> a = j;
        m = glm::vec3(a[0], a[1], a[2]);
    }
    else if (j.is_array() && j.size() == 4)
    {
        Array<float, 4> a = j;
        m = glm::quat(a[0], a[1], a[2], a[3]);
    }
    else if (j.contains("count") && j.contains("id"))
    {
        m = ItemStack(j.contains("id"), j.at("count"));
    }
}

inline void to_json(nlohmann::json& j, const glm::vec2& m)
{
    Array<float, 2> array{m.x, m.y};
    j = array;
}

inline void to_json(nlohmann::json& j, const glm::vec3& m)
{
    Array<float, 3> array{m.x, m.y, m.z};
    j = array;
}

inline void to_json(nlohmann::json& j, const glm::quat& m)
{
    Array<float, 4> array{m.w, m.x, m.y, m.z};
    j = array;
}

inline void to_json(nlohmann::json& j, const Variant& m)
{
    j = m;
}
