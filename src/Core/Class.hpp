#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "Core/Containers/StackVector.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Registry.hpp"

struct ClassHashCode
{
    uint32_t value;
};

inline std::strong_ordering operator<=>(ClassHashCode lhs, ClassHashCode rhs)
{
    return lhs.value <=> rhs.value;
}

inline bool operator==(ClassHashCode lhs, ClassHashCode rhs)
{
    return (lhs <=> rhs) == std::strong_ordering::equal;
}

constexpr ClassHashCode fnv32_class_hash(const char *filename, const char *class_name)
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

    return ClassHashCode(h);
}

#define CLASS(NAME, BASE)                                                                                                                                                                 \
private:                                                                                                                                                                                  \
    static inline const char *s_class_name = #NAME;                                                                                                                                       \
    static inline ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, #NAME);                                                                                                         \
                                                                                                                                                                                          \
    static_assert(sizeof(NAME *) || true);                                                                                                                                                \
                                                                                                                                                                                          \
public:                                                                                                                                                                                   \
    static inline InplaceVector<ClassHashCode, BASE::classes.max_capacity() + 1> classes = InplaceVector<ClassHashCode, BASE::classes.max_capacity() + 1>(BASE::classes, {s_class_hash}); \
                                                                                                                                                                                          \
    static void register_class()                                                                                                                                                          \
    {                                                                                                                                                                                     \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    static View<ClassHashCode> get_static_classes()                                                                                                                                       \
    {                                                                                                                                                                                     \
        return View(classes);                                                                                                                                                             \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    static const char *get_static_class_name()                                                                                                                                            \
    {                                                                                                                                                                                     \
        return s_class_name;                                                                                                                                                              \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    static ClassHashCode get_static_hash_code()                                                                                                                                           \
    {                                                                                                                                                                                     \
        return s_class_hash;                                                                                                                                                              \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    virtual View<ClassHashCode> get_classes() const override                                                                                                                              \
    {                                                                                                                                                                                     \
        return classes;                                                                                                                                                                   \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    virtual const char *get_class_name() const override                                                                                                                                   \
    {                                                                                                                                                                                     \
        return #NAME;                                                                                                                                                                     \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    virtual ClassHashCode get_class_hash_code() const override                                                                                                                            \
    {                                                                                                                                                                                     \
        return s_class_hash;                                                                                                                                                              \
    }                                                                                                                                                                                     \
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
    static inline ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, "Object");

public:
    static inline InplaceVector<ClassHashCode, 1> classes{s_class_hash};

    virtual ~Object() {}

    static const char *get_static_class_name()
    {
        return s_class_name;
    }

    static ClassHashCode get_static_hash_code()
    {
        return s_class_hash;
    }

    virtual View<ClassHashCode> get_classes() const
    {
        return View(classes);
    }

    virtual const char *get_class_name() const
    {
        return s_class_name;
    }

    virtual ClassHashCode get_class_hash_code() const
    {
        return s_class_hash;
    }

    template <typename T>
    bool is() const
    {
        for (const auto& class_hash : get_classes())
        {
            if (class_hash == T::get_static_hash_code())
                return true;
        }
        return false;
    }

    bool is(ClassHashCode hash_code) const
    {
        for (ClassHashCode class_hash : get_classes())
        {
            if (class_hash == hash_code)
                return true;
        }
        return false;
    }
};
