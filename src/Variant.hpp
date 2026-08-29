#pragma once

#include "Color.hpp"
#include "Core/Math.hpp"

#include <nlohmann/json.hpp>

#include <compare>
#include <string>

enum class VariantType : uint32_t
{
    Null = 0,
    /// Boolean primitive, either true or false.
    Bool = 1,
    /// 64-bits precision floating point number.
    Double = 2,
    /// 64-bits signed integer.
    Integer = 3,
    /// String of character.
    String = 4,
    /// 2 dimensional vector.
    Vec2 = 5,
    /// 3 dimensional vector.
    Vec3 = 6,
    /// Quaternion.
    Quat = 7,
    /// Stack of item.
    ItemStack = 8,
    /// Array of variant.
    Array = 9,
    /// Key-Value storage.
    Map = 10,
    Color = 11,
    Point = 12,
};

class ItemStack;
struct Point;

constexpr size_t variant_size = 72;

template <typename T>
struct TypeTag
{
};

template <>
struct TypeTag<std::nullptr_t>
{
    static constexpr VariantType tag = VariantType::Null;
};

template <>
struct TypeTag<bool>
{
    static constexpr VariantType tag = VariantType::Bool;
};

template <>
struct TypeTag<double>
{
    static constexpr VariantType tag = VariantType::Double;
};

template <>
struct TypeTag<int64_t>
{
    static constexpr VariantType tag = VariantType::Integer;
};

template <>
struct TypeTag<std::string>
{
    static constexpr VariantType tag = VariantType::String;
};

template <>
struct TypeTag<glm::dvec2>
{
    static constexpr VariantType tag = VariantType::Vec2;
};

template <>
struct TypeTag<glm::dvec3>
{
    static constexpr VariantType tag = VariantType::Vec3;
};

template <>
struct TypeTag<glm::dquat>
{
    static constexpr VariantType tag = VariantType::Quat;
};

template <>
struct TypeTag<ItemStack>
{
    static constexpr VariantType tag = VariantType::ItemStack;
};

template <typename T>
struct TypeTag<std::vector<T>>
{
    static constexpr VariantType tag = VariantType::Array;
};

template <typename K, typename V>
struct TypeTag<std::map<K, V>>
{
    static constexpr VariantType tag = VariantType::Map;
};

template <>
struct TypeTag<Color>
{
    static constexpr VariantType tag = VariantType::Color;
};

template <>
struct TypeTag<Point>
{
    static constexpr VariantType tag = VariantType::Point;
};

void assert_bad_variant_access(const char *type_name, VariantType got, VariantType expected);

struct __attribute__((aligned(16))) Variant
{
    uint8_t data[variant_size]{0};
    VariantType tag;

    Variant() : tag(VariantType::Null) {}
    Variant(std::nullptr_t) : tag(VariantType::Null) {}

    Variant(bool b) : tag(VariantType::Bool) { data[0] = (uint8_t)b; }
    Variant(double d) : tag(VariantType::Double) { *((double *)data) = d; }
    Variant(int64_t i) : tag(VariantType::Integer) { *((int64_t *)data) = i; }
    Variant(std::string s) : tag(VariantType::String) { new (data) std::string(s); }
    Variant(glm::dvec2 v) : tag(VariantType::Vec2) { *((glm::dvec2 *)data) = v; }
    Variant(glm::dvec3 v) : tag(VariantType::Vec3) { *((glm::dvec3 *)data) = v; }
    Variant(glm::dquat q) : tag(VariantType::Quat) { *((glm::dquat *)data) = q; }
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
    Variant(Variant&& v) noexcept;

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
        assert_bad_variant_access("", tag, VariantType::Map);

        const std::map<Variant, Variant>& map = get_unchecked<std::map<Variant, Variant>>();
        std::map<K, V> v;
        for (const auto& [key, value] : map)
            v[key.get<K>()] = value.get_unchecked<V>();
        return v;
    }

    template <typename K>
    std::map<K, Variant> to_map() const
    {
        assert_bad_variant_access("", tag, VariantType::Map);

        const std::map<Variant, Variant>& map = get_unchecked<std::map<Variant, Variant>>();
        std::map<K, Variant> v;
        for (const auto& [key, value] : map)
            v[key.get<K>()] = value;
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
        static_assert(!std::is_same_v<T, Variant>, "Converting Variant to Variant will always be undefined behavour");
        return *(const T *)data;
    }

    template <typename T>
    T& get_unchecked()
    {
        static_assert(!std::is_same_v<T, Variant>, "Converting Variant to Variant will always be undefined behavour");
        return *(T *)data;
    }

    template <typename T>
    const T& get() const
    {
        assert_bad_variant_access("", tag, TypeTag<T>::tag);
        return *(const T *)data;
    }

    template <typename T>
    T& get()
    {
        assert_bad_variant_access("", tag, TypeTag<T>::tag);
        return *(T *)data;
    }
};
