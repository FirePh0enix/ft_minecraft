#pragma once

#include "Type.hpp"
#include "Variant.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

struct ClassHashCode
{
    uint32_t value;

    constexpr ClassHashCode() : value(0) {}
    constexpr ClassHashCode(uint32_t v) : value(v) {}

    bool operator==(const ClassHashCode& code) const { return value == code.value; }
    bool operator>(const ClassHashCode& code) const { return value > code.value; }
    bool operator<(const ClassHashCode& code) const { return value < code.value; }
};

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

template <const size_t _S>
struct ClassList
{
    std::array<ClassHashCode, _S> m_data;
    size_t m_size;

    constexpr ClassList(std::initializer_list<ClassHashCode> init)
        : m_size(init.size())
    {
        for (size_t i = 0; i < m_size; i++)
            m_data[i] = *(init.begin() + i);
    }

    template <const size_t other_capacity>
    constexpr ClassList(const ClassList<other_capacity>& vec, const std::initializer_list<ClassHashCode>& list)
        : m_data{0}, m_size(vec.size() + list.size())
    {
        size_t i = 0;
        for (; i < vec.size(); i++)
            m_data[i] = vec.m_data[i];

        for (size_t j = 0; j < list.size(); i++, j++)
            m_data[i] = *(list.begin() + j);
    }

    constexpr size_t size() const { return m_size; }
    constexpr size_t max_capacity() const { return _S; }
};

#define CLASS(NAME, BASE)                                                                                                                           \
private:                                                                                                                                            \
    static constexpr ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, #NAME);                                                                \
                                                                                                                                                    \
public:                                                                                                                                             \
    static inline ClassList<BASE::classes.max_capacity() + 1> classes = ClassList<BASE::classes.max_capacity() + 1>(BASE::classes, {s_class_hash}); \
    static inline Type type;                                                                                                                        \
                                                                                                                                                    \
    static std::span<const ClassHashCode> get_static_classes()                                                                                      \
    {                                                                                                                                               \
        return classes.m_data;                                                                                                                      \
    }                                                                                                                                               \
                                                                                                                                                    \
    static const char *get_static_class_name()                                                                                                      \
    {                                                                                                                                               \
        return #NAME;                                                                                                                               \
    }                                                                                                                                               \
                                                                                                                                                    \
    static constexpr ClassHashCode get_static_hash_code()                                                                                           \
    {                                                                                                                                               \
        return s_class_hash;                                                                                                                        \
    }                                                                                                                                               \
                                                                                                                                                    \
    virtual std::span<const ClassHashCode> get_classes() const override                                                                             \
    {                                                                                                                                               \
        return classes.m_data;                                                                                                                      \
    }                                                                                                                                               \
                                                                                                                                                    \
    virtual const char *get_class_name() const override                                                                                             \
    {                                                                                                                                               \
        return #NAME;                                                                                                                               \
    }                                                                                                                                               \
                                                                                                                                                    \
    virtual ClassHashCode get_class_hash_code() const override                                                                                      \
    {                                                                                                                                               \
        return s_class_hash;                                                                                                                        \
    }                                                                                                                                               \
                                                                                                                                                    \
    virtual Type *get_type() const override                                                                                                         \
    {                                                                                                                                               \
        return &type;                                                                                                                               \
    }                                                                                                                                               \
                                                                                                                                                    \
private:

template <typename T>
concept IsObject = requires() {
    { T::register_class() } -> std::same_as<void>;
    { T::get_static_classes() } -> std::same_as<std::span<const ClassHashCode>>;
    { T::get_static_class_name() } -> std::same_as<const char *>;
    { T::get_static_hash_code() } -> std::same_as<ClassHashCode>;
};

class Object
{
    static inline ClassHashCode s_class_hash = fnv32_class_hash(__FILE__, "Object");

public:
    static inline ClassList<1> classes{s_class_hash};
    static inline Type type;

    virtual ~Object() {}

    static const char *get_static_class_name()
    {
        return "Object";
    }

    static ClassHashCode get_static_hash_code()
    {
        return s_class_hash;
    }

    virtual std::span<const ClassHashCode> get_classes() const
    {
        return classes.m_data;
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

    virtual Type *get_type() const
    {
        return &type;
    }

    Variant call(std::string_view name, std::span<const Variant> args)
    {
        return get_type()->call(name, this, args);
    }

    void set(std::string_view name, Variant value)
    {
        get_type()->set(name, this, value);
    }

    Variant get(std::string_view name)
    {
        return get_type()->get(name, this);
    }
};
