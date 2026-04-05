#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "Core/Containers/InplaceVector.hpp"
#include "Core/Containers/View.hpp"

struct ClassHashCode
{
    uint32_t value;

    constexpr ClassHashCode() : value(0) {}
    constexpr ClassHashCode(uint32_t v) : value(v) {}
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
    static constexpr ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, #NAME);                                                                                                      \
                                                                                                                                                                                          \
public:                                                                                                                                                                                   \
    static inline InplaceVector<ClassHashCode, BASE::classes.max_capacity() + 1> classes = InplaceVector<ClassHashCode, BASE::classes.max_capacity() + 1>(BASE::classes, {s_class_hash}); \
                                                                                                                                                                                          \
    static View<ClassHashCode> get_static_classes()                                                                                                                                       \
    {                                                                                                                                                                                     \
        return View(classes);                                                                                                                                                             \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    static const char *get_static_class_name()                                                                                                                                            \
    {                                                                                                                                                                                     \
        return #NAME;                                                                                                                                                                     \
    }                                                                                                                                                                                     \
                                                                                                                                                                                          \
    static constexpr ClassHashCode get_static_hash_code()                                                                                                                                 \
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
concept IsObject = requires() {
    { T::register_class() } -> std::same_as<void>;
    { T::get_static_classes() } -> std::same_as<View<ClassHashCode>>;
    { T::get_static_class_name() } -> std::same_as<const char *>;
    { T::get_static_hash_code() } -> std::same_as<ClassHashCode>;
};

class Object
{
    static inline ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, "Object");

public:
    static inline InplaceVector<ClassHashCode, 1> classes{s_class_hash};

    virtual ~Object() {}

    static const char *get_static_class_name()
    {
        return "Object";
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
        return get_static_class_name();
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
