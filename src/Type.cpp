#include "Type.hpp"
#include "Core/Class.hpp"
#include "Core/Containers/InplaceVector.hpp"
#include "Core/Containers/View.hpp"
#include "Variant.hpp"

Variant Type::call(String name, Object *instance, View<Variant> args)
{
    if (m_methods.contains(name))
        return m_methods[name].func(instance, Arguments{.args = args});
    return m_parent ? m_parent->call(name, instance, args) : nullptr;
}

void Type::set(String name, Object *instance, Variant value)
{
    if (m_properties.contains(name))
    {
        InplaceVector<Variant, 1> args;
        args.push_back(value);

        Property property = m_properties[name];
        property.setter(instance, Arguments{.args = args});
    }
    else if (m_parent)
    {
        m_parent->set(name, instance, value);
    }
}

Variant Type::get(String name, Object *instance)
{
    if (m_properties.contains(name))
    {
        Property property = m_properties[name];
        return property.getter(instance, {});
    }
    return m_parent ? m_parent->get(name, instance) : nullptr;
}
