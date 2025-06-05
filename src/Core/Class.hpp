#pragma once

#define STRINGIFY_CLASSNAME(NAME) #NAME

#define CLASS(NAME, BASE)                                               \
private:                                                                \
    static inline std::vector<const char *> s_classes;                  \
    static inline const char *s_class_name = STRINGIFY_CLASSNAME(NAME); \
                                                                        \
    static_assert(sizeof(NAME *) || true);                              \
                                                                        \
public:                                                                 \
    static void register_class()                                        \
    {                                                                   \
        for (const auto& iter : BASE::get_static_classes())             \
            s_classes.push_back(iter);                                  \
        s_classes.push_back(s_class_name);                              \
    }                                                                   \
                                                                        \
    static const std::vector<const char *> get_static_classes()         \
    {                                                                   \
        return s_classes;                                               \
    }                                                                   \
                                                                        \
    static const char *get_static_class_name()                          \
    {                                                                   \
        return s_class_name;                                            \
    }                                                                   \
                                                                        \
    template <typename T>                                               \
    static bool is_instance_of()                                        \
    {                                                                   \
        for (const auto& class_name : s_classes)                        \
        {                                                               \
            if (strcmp(class_name, T::get_static_class_name()) == 0)    \
                return true;                                            \
        }                                                               \
        return false;                                                   \
    }                                                                   \
                                                                        \
    static bool is_instance_of(const char *other_class_name)            \
    {                                                                   \
        for (const auto& class_name : s_classes)                        \
        {                                                               \
            if (strcmp(class_name, other_class_name) == 0)              \
                return true;                                            \
        }                                                               \
        return false;                                                   \
    }                                                                   \
                                                                        \
    virtual const char *get_class_name() const override                 \
    {                                                                   \
        return s_class_name;                                            \
    }                                                                   \
                                                                        \
private:

class Object
{
    static inline std::vector<const char *> s_classes;
    static inline const char *s_class_name = "Object";

public:
    static void register_class()
    {
        s_classes.push_back(s_class_name);
    }

    static const std::vector<const char *> get_static_classes()
    {
        return s_classes;
    }

    static const char *get_static_class_name()
    {
        return s_class_name;
    }

    virtual const char *get_class_name() const
    {
        return s_class_name;
    }
};
