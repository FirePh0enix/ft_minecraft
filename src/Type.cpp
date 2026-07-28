#include "Type.hpp"
#include "Core/Class.hpp"
#include "Variant.hpp"

Variant Type::call(std::string_view name, Object *instance, std::span<const Variant> args)
{
    if (m_methods.contains(name))
        return m_methods.find(name)->second.func(instance, Arguments{.args = args});
    return m_parent ? m_parent->call(name, instance, args) : nullptr;
}

void Type::set(std::string_view name, Object *instance, Variant value)
{
    if (m_properties.contains(name))
    {
        std::array<Variant, 1> args{value};

        Property property = m_properties.find(name)->second;
        property.setter(instance, Arguments{.args = args});
    }
    else if (m_parent)
    {
        m_parent->set(name, instance, value);
    }
}

Variant Type::get(std::string_view name, Object *instance)
{
    if (m_properties.contains(name))
    {
        Property property = m_properties.find(name)->second;
        return property.getter(instance, {});
    }
    return m_parent ? m_parent->get(name, instance) : nullptr;
}
