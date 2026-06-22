#include "Variant.hpp"

#include "Item/ItemStack.hpp"

#include <compare>

Variant::Variant(ItemStack is) : tag(VariantType::ItemStack)
{
    new (data) ItemStack(is);
}

Variant::Variant(const Variant& v)
    : tag(v.tag)
{
    if (v.has(VariantType::String))
    {
        const String& s = v.get_unchecked<String>();
        new (data) String(s);
    }
    else if (v.has(VariantType::ItemStack))
    {
        const ItemStack& s = v.get_unchecked<ItemStack>();
        new (data) ItemStack(s);
    }
    else if (v.has(VariantType::Array))
    {
        const Vector<Variant>& s = v.get_unchecked<Vector<Variant>>();
        new (data) Vector<Variant>(s);
    }
    else if (v.has(VariantType::Map))
    {
        const Map<Variant, Variant>& m = v.get_unchecked<Map<Variant, Variant>>();
        new (data) Map<Variant, Variant>(m);
    }
    else
    {
        memcpy(data, v.data, 32);
    }
}

Variant::~Variant()
{
    if (has(VariantType::String))
    {
        ((String *)data)->~String();
    }
    else if (has(VariantType::ItemStack))
    {
        ((ItemStack *)data)->~ItemStack();
    }
    else if (has(VariantType::Array))
    {
        ((Vector<Variant> *)data)->~Vector<Variant>();
    }
    else if (has(VariantType::Map))
    {
        ((Map<Variant, Variant> *)data)->~Map<Variant, Variant>();
    }
}

std::strong_ordering Variant::operator<=>(const Variant& variant) const
{
    if (tag != variant.tag)
        return tag <=> variant.tag;

    switch (tag)
    {
    case VariantType::Null:
        return std::strong_ordering::equal;
    case VariantType::Bool:
        return get_unchecked<bool>() <=> variant.get_unchecked<bool>();
    case VariantType::Double:
        std::abort();
        // return get_unchecked<double>() <=> variant.get_unchecked<double>();
    case VariantType::Integer:
        return get_unchecked<int64_t>() <=> variant.get_unchecked<int64_t>();
    case VariantType::String:
        return get_unchecked<String>() <=> variant.get_unchecked<String>();
    case VariantType::Vec2: // TODO: implement this
    case VariantType::Vec3:
    case VariantType::Quat:
    case VariantType::ItemStack:
    case VariantType::Array:
    case VariantType::Map:
        std::abort();
    }

    return std::strong_ordering::equal;
}

void from_json(const nlohmann::json& j, Variant& m)
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
        m = ItemStack(Id<Item>(j.at("id")), j.at("count"));
    }
}
