#pragma once

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

#define CLASS(NAME, BASE)                                                                    \
private:                                                                                     \
    static inline const char *s_class_name = #NAME;                                          \
    static inline uint32_t s_class_hash = fnv32_class_hash(__FILE__, #NAME);                 \
    static inline std::vector<uint32_t> s_classes;                                           \
                                                                                             \
    static_assert(sizeof(NAME *) || true);                                                   \
    static_assert(sizeof(NAME *) || true);                                                   \
                                                                                             \
public:                                                                                      \
    static void register_class()                                                             \
    {                                                                                        \
        if (s_classes.size() > 0)                                                            \
            return;                                                                          \
                                                                                             \
        for (const auto& iter : BASE::get_static_classes())                                  \
            s_classes.push_back(iter);                                                       \
                                                                                             \
        s_classes.push_back(s_class_hash);                                                   \
    }                                                                                        \
                                                                                             \
    static const std::vector<uint32_t>& get_static_classes()                                 \
    {                                                                                        \
        assert(s_classes.size() > 0 && "Class was not registered (" #NAME " <= " #BASE ")"); \
        return s_classes;                                                                    \
    }                                                                                        \
                                                                                             \
    static const char *get_static_class_name()                                               \
    {                                                                                        \
        return s_class_name;                                                                 \
    }                                                                                        \
                                                                                             \
    static uint32_t get_static_hash_code()                                                   \
    {                                                                                        \
        return s_class_hash;                                                                 \
    }                                                                                        \
                                                                                             \
    template <typename T>                                                                    \
    bool is()                                                                                \
    {                                                                                        \
        for (const auto& class_name : get_classes())                                         \
        {                                                                                    \
            if (class_name == T::get_static_hash_code())                                     \
                return true;                                                                 \
        }                                                                                    \
        return false;                                                                        \
    }                                                                                        \
                                                                                             \
    virtual const std::vector<uint32_t>& get_classes() const override                        \
    {                                                                                        \
        return s_classes;                                                                    \
    }                                                                                        \
                                                                                             \
    virtual const char *get_class_name() const override                                      \
    {                                                                                        \
        return #NAME;                                                                        \
    }                                                                                        \
                                                                                             \
private:

class Object
{
    static inline const char *s_class_name = "Object";
    static inline uint32_t s_class_hash = fnv32_class_hash(__FILE__, "Object");
    static inline std::vector<uint32_t> s_classes{s_class_hash};

public:
    virtual ~Object() {}

    static const std::vector<uint32_t>& get_static_classes()
    {
        return s_classes;
    }

    static const char *get_static_class_name()
    {
        return s_class_name;
    }

    static uint32_t get_static_hash_code()
    {
        return s_class_hash;
    }

    virtual const std::vector<uint32_t>& get_classes() const
    {
        return s_classes;
    }

    virtual const char *get_class_name() const
    {
        return s_class_name;
    }

    template <typename T>
    bool is()
    {
        for (const auto& class_name : get_classes())
        {
            if (class_name == T::get_static_hash_code())
                return true;
        }
        return false;
    }
};
