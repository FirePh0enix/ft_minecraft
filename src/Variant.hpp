#pragma once

#include "Color.hpp"
#include "Core/Math.hpp"

#include <nlohmann/json.hpp>

#include <compare>
#include <string>

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
    Color,
    Point,
};

class ItemStack;
struct Point;

struct __attribute__((aligned(16))) Variant
{
    uint8_t data[64]{0};
    VariantType tag;

    Variant() : tag(VariantType::Null) {}
    Variant(std::nullptr_t) : tag(VariantType::Null) {}

    Variant(bool b) : tag(VariantType::Bool) { data[0] = (uint8_t)b; }
    Variant(double d) : tag(VariantType::Double) { *((double *)data) = d; }
    Variant(int64_t i) : tag(VariantType::Integer) { *((int64_t *)data) = i; }
    Variant(std::string_view s) : tag(VariantType::String) { new (data) std::string(s); }
    Variant(const std::string& s) : tag(VariantType::String) { new (data) std::string(s); }
    Variant(glm::vec2 v) : tag(VariantType::Vec2) { *((glm::vec2 *)data) = v; }
    Variant(glm::vec3 v) : tag(VariantType::Vec3) { *((glm::vec3 *)data) = v; }
    Variant(glm::quat q) : tag(VariantType::Quat) { *((glm::quat *)data) = q; }
    Variant(ItemStack is);
    Variant(Color color) : tag(VariantType::Color) { *((Color *)data) = color; }
    Variant(Point point);

    template <typename T>
    Variant(const std::span<T>& values)
        : tag(VariantType::Array)
    {
        auto v = new (data) std::vector<Variant>();
        v->reserve(values.size());

        for (const auto& value : values)
            v->push_back(Variant(value));
    }

    Variant(const std::vector<Variant>& values)
        : tag(VariantType::Array)
    {
        new (data) std::vector<Variant>(values);
    }

    template <typename K, typename V>
    Variant(const std::map<K, V>& map)
        : tag(VariantType::Map)
    {
        new (data) std::map<Variant, Variant>();
        std::map<Variant, Variant>& m = get_unchecked<std::map<Variant, Variant>>();
        for (const auto& [key, value] : map)
            m[Variant(key)] = Variant(value);
    }

    Variant(const std::map<Variant, Variant>& map)
        : tag(VariantType::Map)
    {
        new (data) std::map<Variant, Variant>(map);
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
    std::vector<T> to_array() const
    {
        const std::vector<Variant>& array = get_unchecked<std::vector<Variant>>();
        std::vector<T> v;
        v.reserve(array.size());

        for (size_t i = 0; i < array.size(); i++)
            v.push_back(array[i].get_unchecked<T>());

        return v;
    }

    template <typename K, typename V>
    std::map<K, V> to_map() const
    {
        const std::map<Variant, Variant>& map = get_unchecked<std::map<Variant, Variant>>();
        std::map<K, V> v;

        for (const auto& [key, value] : map)
            v[key.get_unchecked<K>()] = value.get_unchecked<V>();

        return v;
    }

    constexpr bool has(VariantType expected) const { return tag == expected; }

    std::strong_ordering operator<=>(const Variant& variant) const;
    bool operator==(const Variant& variant) const { return *this <=> variant == std::strong_ordering::equal; }
    bool operator!=(const Variant& variant) const { return !(*this == variant); }
    bool operator<(const Variant& variant) const { return *this <=> variant == std::strong_ordering::less; }
    bool operator>(const Variant& variant) const { return *this <=> variant == std::strong_ordering::greater; }

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
};
