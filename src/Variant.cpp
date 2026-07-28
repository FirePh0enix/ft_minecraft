#include "Variant.hpp"

#include "Item/ItemStack.hpp"

#include <compare>

Variant::Variant(ItemStack is)
    : tag(VariantType::ItemStack)
{
    new (data) ItemStack(is);
}

Variant::Variant(const Variant& v)
    : tag(v.tag)
{
    if (v.has(VariantType::String))
    {
        const std::string& s = v.get_unchecked<std::string>();
        new (data) std::string(s);
    }
    else if (v.has(VariantType::ItemStack))
    {
        const ItemStack& s = v.get_unchecked<ItemStack>();
        new (data) ItemStack(s);
    }
    else if (v.has(VariantType::Array))
    {
        const std::vector<Variant>& s = v.get_unchecked<std::vector<Variant>>();
        new (data) std::vector<Variant>(s);
    }
    else if (v.has(VariantType::Map))
    {
        const std::map<Variant, Variant>& m = v.get_unchecked<std::map<Variant, Variant>>();
        new (data) std::map<Variant, Variant>(m);
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
        ((std::string *)data)->~basic_string();
    }
    else if (has(VariantType::ItemStack))
    {
        ((ItemStack *)data)->~ItemStack();
    }
    else if (has(VariantType::Array))
    {
        ((std::vector<Variant> *)data)->~vector<Variant>();
    }
    else if (has(VariantType::Map))
    {
        ((std::map<Variant, Variant> *)data)->~map<Variant, Variant>();
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
        return get_unchecked<std::string>() <=> variant.get_unchecked<std::string>();
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
