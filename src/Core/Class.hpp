#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <variant>

#include "Core/Containers/StackVector.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Math.hpp"
#include "Core/Ref.hpp"
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
        ClassRegistry::get().register_class<NAME>();                                                                                                                                      \
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

enum class PrimitiveType
{
    Bool,

    Float,
    Vec2,
    Vec3,
    Vec4,
    Quat,
};

struct Variant : public std::variant<std::nullptr_t, bool, float, glm::vec2, glm::vec3, glm::vec4, glm::quat>
{
    template <typename T>
    operator T() { return std::get<T>(*this); }

    template <typename T>
    constexpr bool is() { return std::holds_alternative<T>(); }
};

template <typename T = Object>
using PropertyGetter = Variant (*)(T *);

template <typename T = Object>
using PropertySetter = void (*)(T *, Variant);

struct Property
{
    PrimitiveType type;
    PropertyGetter<> getter;
    PropertySetter<> setter;
};

struct Class
{
    std::map<std::string, Property> properties;
};

template <typename T>
concept HasBindMethods = requires() {
    { T::bind_methods() } -> std::same_as<void>;
};

class ClassRegistry
{
public:
    static ClassRegistry& get()
    {
        static ClassRegistry singleton;
        return singleton;
    }

    template <typename T>
    void register_class()
    {
        m_classes[T::get_static_hash_code()] = Class{};
    }

    template <typename T>
        requires HasBindMethods<T>
    void register_class()
    {
        m_classes[T::get_static_hash_code()] = Class{};
        T::bind_methods();
    }

    template <typename T>
    void register_property(const std::string& property_name, PrimitiveType type, PropertyGetter<T> getter, PropertySetter<T> setter)
    {
        m_classes[T::get_static_hash_code()].properties[property_name] = Property{.type = type, .getter = reinterpret_cast<PropertyGetter<>>(getter), .setter = reinterpret_cast<PropertySetter<>>(setter)};
    }

    const Class& get_class(ClassHashCode hash) const
    {
        return m_classes.at(hash);
    }

private:
    std::map<ClassHashCode, Class> m_classes;
};
