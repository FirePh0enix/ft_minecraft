#pragma once

#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Result.hpp"
#include "Core/String.hpp"
#include "Variant.hpp"

#include <functional>

class Object;

struct Arguments
{
    View<Variant> args;
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
    Result<void> add_method(String name, Ret (T::*func)(Args...))
    {
        TRY(m_methods.put(name, Method{.func = [&](Object *instance, Arguments args)
                                       {
                                           constexpr bool is_null = std::is_same_v<Ret, void>;
                                           if constexpr (is_null)
                                           {
                                               (reinterpret_cast<T *>(instance)->*func)(args.pop<Args>()...);
                                               return nullptr;
                                           }
                                           else
                                           {
                                               Variant ret = (reinterpret_cast<T *>(instance)->*func)(args.pop<Args>()...);
                                               return ret;
                                           }
                                       }}));
        return Result<void>();
    }

    template <typename T, typename Value>
    Result<void> add_property(String name, Value (T::*getter)() const, void (T::*setter)(Value v))
    {
        TRY(m_properties.put(name, Property{
                                       .getter = [&](Object *instance, Arguments args)
                                       { (void) args; return (reinterpret_cast<T *>(instance)->*getter)(); },
                                       .setter = [&](Object *instance, Arguments args)
                                       { (reinterpret_cast<T *>(instance)->*setter)(args.pop<Value>()); return nullptr; },
                                   }));
        return Result<void>();
    }

    Variant call(String name, Object *instance, View<Variant> args);
    void set(String name, Object *instance, Variant value);
    Variant get(String name, Object *instance);

    const Method& get_method(StringView name) const
    {
        if (m_methods.contains(name))
            return *m_methods.get_ptr(name).get();
        ASSERT_V(m_parent != nullptr, "");
        return m_parent->get_method(name);
    }

    const Property& get_property(StringView name) const
    {
        if (m_properties.contains(name))
            return *m_properties.get_ptr(name).get();
        ASSERT_V(m_parent != nullptr, "");
        return m_parent->get_property(name);
    }

private:
    HashMap<String, Method> m_methods;
    HashMap<String, Property> m_properties;
    Type *m_parent;
};
