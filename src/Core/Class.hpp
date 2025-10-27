#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "Core/Containers/StackVector.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Registry.hpp"

constexpr uint32_t fnv32_class_hash(const char *filename, const char *class_name)
{
    size_t filename_len = 0;
    while (filename[filename_len])
        filename_len++;

    size_t class_name_len = 0;
    while (class_name[class_name_len])
        class_name_len++;

    const uint32_t fnv_32_prime = 0x01000193;
    uint32_t h = 0x811c9dc5;

    while (filename_len--)
    {
        h ^= *filename++;
        h *= fnv_32_prime;
    }

    while (class_name_len--)
    {
        h ^= *class_name++;
        h *= fnv_32_prime;
    }

    return h;
}

#define CLASS(NAME, BASE)                                                                                                                                                       \
private:                                                                                                                                                                        \
    static inline const char *s_class_name = #NAME;                                                                                                                             \
    static inline uint32_t s_class_hash = fnv32_class_hash(__FILE__, #NAME);                                                                                                    \
                                                                                                                                                                                \
    static_assert(sizeof(NAME *) || true);                                                                                                                                      \
                                                                                                                                                                                \
public:                                                                                                                                                                         \
    static inline InplaceVector<uint32_t, BASE::classes.max_capacity() + 1> classes = InplaceVector<uint32_t, BASE::classes.max_capacity() + 1>(BASE::classes, {s_class_hash}); \
                                                                                                                                                                                \
    static void register_class()                                                                                                                                                \
    {                                                                                                                                                                           \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    static View<uint32_t> get_static_classes()                                                                                                                                  \
    {                                                                                                                                                                           \
        return View(classes);                                                                                                                                                   \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    static const char *get_static_class_name()                                                                                                                                  \
    {                                                                                                                                                                           \
        return s_class_name;                                                                                                                                                    \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    static uint32_t get_static_hash_code()                                                                                                                                      \
    {                                                                                                                                                                           \
        return s_class_hash;                                                                                                                                                    \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    template <typename T>                                                                                                                                                       \
    bool is() const                                                                                                                                                             \
    {                                                                                                                                                                           \
        for (const auto& class_name : get_classes())                                                                                                                            \
        {                                                                                                                                                                       \
            if (class_name == T::get_static_hash_code())                                                                                                                        \
                return true;                                                                                                                                                    \
        }                                                                                                                                                                       \
        return false;                                                                                                                                                           \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    virtual View<uint32_t> get_classes() const override                                                                                                                         \
    {                                                                                                                                                                           \
        return classes;                                                                                                                                                         \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
    virtual const char *get_class_name() const override                                                                                                                         \
    {                                                                                                                                                                           \
        return #NAME;                                                                                                                                                           \
    }                                                                                                                                                                           \
                                                                                                                                                                                \
private:

template <typename T>
struct TypeInfo;

#define STRUCT(NAME)                                                                             \
    template <>                                                                                  \
    struct TypeInfo<NAME>                                                                        \
    {                                                                                            \
        static inline const char *struct_name = #NAME;                                           \
                                                                                                 \
        static void register_type()                                                              \
        {                                                                                        \
            CoreRegistry::get().register_struct(struct_name, Struct(struct_name, sizeof(NAME))); \
        }                                                                                        \
    }

#define STRUCTNAME(TYPE) TypeInfo<TYPE>::struct_name

class Object
{
    static inline const char *s_class_name = "Object";
    static inline uint32_t s_class_hash = fnv32_class_hash(__FILE__, "Object");

public:
    static inline InplaceVector<uint32_t, 1> classes{s_class_hash};

    virtual ~Object() {}

    static const char *get_static_class_name()
    {
        return s_class_name;
    }

    static uint32_t get_static_hash_code()
    {
        return s_class_hash;
    }

    virtual View<uint32_t> get_classes() const
    {
        return View(classes);
    }

    virtual const char *get_class_name() const
    {
        return s_class_name;
    }

    template <typename T>
    bool is() const
    {
        for (const auto& class_name : get_classes())
        {
            if (class_name == T::get_static_hash_code())
                return true;
        }
        return false;
    }
};
