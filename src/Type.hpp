#pragma once

#include "Core/Assert.hpp"
#include "Variant.hpp"
#include "stdext.hpp"

#include <functional>

class Object;

struct Arguments
{
    std::span<const Variant> args;
    size_t i = 0;

    template <typename T>
    T pop()
    {
        ASSERT_V(i <= args.size(), "");
        return args[i++].get_unchecked<T>();

    }
};

class Type
{
public:
    using MethodFunc = std::function<Variant(Object *, Arguments)>;

    struct Method
    {
        MethodFunc func;
    };

    struct Property
    {
        MethodFunc getter;
        MethodFunc setter;
    };

    Type()
        : m_parent(nullptr)
    {
    }

    Type(Type *parent)
        : m_parent(parent)
    {
    }

    template <typename T, typename Ret, typename... Args>
    void add_method(std::string_view name, Ret (T::*func)(Args...))
    {
        m_methods[std::string(name)] = Method{
            .func = [func](Object *instance, Arguments args)
            {
                constexpr bool ret_is_void = std::is_same_v<Ret, void>;
                if constexpr (ret_is_void)
                {
                    (reinterpret_cast<T *>(instance)->*func)(args.pop<Args>()...);
                    return nullptr;
                }
                else
                {
                    Variant ret = (reinterpret_cast<T *>(instance)->*func)(args.pop<Args>()...);
                    return ret;
                }
            }};
    }

    template <typename T, typename Value>
    void add_property(std::string_view name, Value (T::*getter)() const, void (T::*setter)(Value v))
    {
        m_properties[std::string(name)] = Property{
            .getter = [getter](Object *instance, Arguments args)
            {
            (void)args;
            return (reinterpret_cast<T *>(instance)->*getter)(); },
            .setter = [setter](Object *instance, Arguments args)
            {
            (reinterpret_cast<T *>(instance)->*setter)(args.pop<Value>());
            return nullptr; },
        };
    }

    Variant call(std::string_view name, Object *instance, std::span<const Variant> args);
    void set(std::string_view name, Object *instance, Variant value);
    Variant get(std::string_view name, Object *instance);

    const Method& get_method(std::string_view name) const
    {
        auto iter = m_methods.find(name);
        if (iter != m_methods.end())
            return iter->second;
        ASSERT_V(m_parent != nullptr, "");
        return m_parent->get_method(name);
    }

    const Property& get_property(std::string_view name) const
    {
        auto iter = m_properties.find(name);
        if (iter != m_properties.end())
            return iter->second;
        ASSERT_V(m_parent != nullptr, "");
        return m_parent->get_property(name);
    }

private:
    stdext::string_map<Method> m_methods;
    stdext::string_map<Property> m_properties;
    Type *m_parent;
};
