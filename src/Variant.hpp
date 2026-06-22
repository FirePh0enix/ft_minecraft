#pragma once

#include "Core/Containers/Array.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Json.hpp"
#include "Core/Math.hpp"
#include "Core/String.hpp"

#include <compare>
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
    /**
     * Array of variants of one specific type.
     */
    Array,
    /**
     * Key value storage.
     */
    Map,
};

class ItemStack;

struct __attribute__((aligned(16))) Variant
{
    uint8_t data[40]{0};
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
    Variant(ItemStack is);

    template <typename T>
    Variant(const View<T>& values)
        : tag(VariantType::Array)
    {
        auto v = new (data) Vector<Variant>();
        v->reserve(values.size());

        for (const auto& value : values)
            v->append(Variant(value));
    }

    Variant(const Vector<Variant>& values)
        : tag(VariantType::Array)
    {
        new (data) Vector<Variant>(values);
    }

    template <typename K, typename V>
    Variant(const Map<K, V>& map)
        : tag(VariantType::Map)
    {
        new (data) Map<Variant, Variant>();
        Map<Variant, Variant>& m = get_unchecked<Map<Variant, Variant>>();
        for (const auto& [key, value] : map)
            m.put(Variant(key), Variant(value));
    }

    Variant(const Map<Variant, Variant>& map)
        : tag(VariantType::Map)
    {
        new (data) Map<Variant, Variant>(map);
    }

    Variant(const Variant& v);

    ~Variant();

    Variant& operator=(const Variant& v)
    {
        this->~Variant();
        new (this) Variant(v);
        return *this;
    }

    template <typename T>
    Vector<T> to_array() const
    {
        const Vector<Variant>& array = get_unchecked<Vector<Variant>>();
        Vector<T> v;
        v.reserve(array.size());

        for (size_t i = 0; i < array.size(); i++)
            v.append(array.get_unchecked(i).get_unchecked<T>());

        return v;
    }

    template <typename K, typename V>
    Map<K, V> to_map() const
    {
        const Map<Variant, Variant>& map = get_unchecked<Map<Variant, Variant>>();
        Map<K, V> v;

        for (const auto& [key, value] : map)
            v.put(key.get_unchecked<K>(), value.get_unchecked<V>());

        return v;
    }

    constexpr bool has(VariantType expected) const { return tag == expected; }

    template <typename T>
    const T& get_unchecked() const
    {
        return *(const T *)data;
    }

    template <typename T>
    T& get_unchecked()
    {
        return *(T *)data;
    }

    std::strong_ordering operator<=>(const Variant& variant) const;
    bool operator==(const Variant& variant) const { return *this <=> variant == std::strong_ordering::equal; }
    bool operator!=(const Variant& variant) const { return !(*this == variant); }
    bool operator<(const Variant& variant) const { return *this <=> variant == std::strong_ordering::less; }
    bool operator>(const Variant& variant) const { return *this <=> variant == std::strong_ordering::greater; }
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

void from_json(const nlohmann::json& j, Variant& m);

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
