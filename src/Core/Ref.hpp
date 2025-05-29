#pragma once

template <typename T>
class Ref
{
public:
    using ReferenceType = uint32_t;

    Ref()
        : m_value(nullptr), m_references(nullptr)
    {
    }

    Ref(std::nullptr_t)
        : m_value(nullptr), m_references(nullptr)
    {
    }

    Ref(T *value)
        : m_value(value), m_references(new ReferenceType(1))
    {
    }

    Ref(const Ref& other)
        : m_value(other.m_value), m_references(other.references())
    {
        ref();
    }

    template <typename Parent, typename = std::is_base_of<Parent, T>::value>
    Ref(const Ref<Parent>& other)
        : m_value((T *)other.ptr()), m_references(other.references())
    {
        ref();
    }

    Ref(T *value, ReferenceType *references)
        : m_value(value), m_references(references)
    {
    }

    ~Ref()
    {
        if (!is_null())
            unref();
    }

    void operator=(std::nullptr_t)
    {
        unref();

        m_value = nullptr;
        m_references = nullptr;
    }

    Ref& operator=(const Ref& other)
    {
        if (m_value == other.m_value && m_references == other.m_references)
        {
            return *this;
        }

        if (!is_null())
        {
            unref();
        }

        m_value = other.m_value;
        m_references = other.m_references;

        ref();

        return *this;
    }

    T *operator->()
    {
        return m_value;
    }

    const T *operator->() const
    {
        return m_value;
    }

    T& operator*()
    {
        return *m_value;
    }

    const T& operator*() const
    {
        return *m_value;
    }

    template <typename Subclass>
    Ref<Subclass> cast_to() const
    {
        return Ref<Subclass>(static_cast<Subclass *>(m_value), m_references);
    }

    bool is_null() const
    {
        return m_value == nullptr;
    }

    inline T *ptr() const
    {
        return m_value;
    }

    inline ReferenceType *references() const
    {
        return m_references;
    }

private:
    T *m_value;
    ReferenceType *m_references;

    void ref()
    {
        *m_references += 1;
    }

    void unref()
    {
        // *m_references -= 1;

        // if (*m_references == 0)
        // {
        //     delete m_value;
        //     delete m_references;
        // }
    }
};

template <typename T, typename... Args>
inline Ref<T> make_ref(Args... args)
{
    return Ref<T>(new T(args...));
}
