#include "Variant.hpp"
#include "Engine.hpp"
#include "Item/ItemStack.hpp"

Variant::Variant(ItemStack is) : tag(VariantType::ItemStack)
{
    *((ItemStack *)data) = is;
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
