#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "Core/StringView.hpp"

template <typename T>
struct TypeInfo;

struct Struct
{
    const char *name;
    uint32_t size;
};

class CoreRegistry
{
public:
    static inline CoreRegistry& get()
    {
        return s_singleton;
    }

    void register_struct(const char *name, Struct s);

    Struct get_struct(const StringView& name);

private:
    static CoreRegistry s_singleton;
    std::map<std::string, Struct> m_structs;
};

template <typename... T>
inline void register_classes_internal()
{
    (T::register_class(), ...);
}

#define REGISTER_CLASSES(...) register_classes_internal<__VA_ARGS__>()

template <typename... T>
inline void register_structs_internal()
{
    (TypeInfo<T>::register_type(), ...);
}

#define REGISTER_STRUCTS(...) register_structs_internal<__VA_ARGS__>()
