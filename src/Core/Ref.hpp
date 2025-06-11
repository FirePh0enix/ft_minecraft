#pragma once

#include "Core/Error.hpp"

template <typename T>
class Ref
{
public:
    using ReferenceType = uint32_t;

    constexpr Ref()
        : m_value(nullptr), m_references(nullptr)
    {
    }

    constexpr Ref(std::nullptr_t)
        : m_value(nullptr), m_references(nullptr)
    {
    }

    Ref(T *value)
        : m_value(value), m_references(new ReferenceType(1))
    {
    }

    Ref(const Ref& other)
    {
        this->operator=(other);
    }

    Ref(Ref&& other)
    {
        m_value = other.m_value;
        other.m_value = nullptr;
        other.m_references = nullptr;
    }

    Ref(T *value, ReferenceType *references)
        : m_value(value), m_references(references)
    {
    }

    ~Ref()
    {
        unref();

        m_value = nullptr;
        m_references = nullptr;
    }

    void operator=(std::nullptr_t)
    {
        unref();

        m_value = nullptr;
        m_references = nullptr;
    }

    void operator=(const Ref& other)
    {
        if (m_value == other.m_value)
        {
            return;
        }

        m_value = other.m_value;
        m_references = other.m_references;

        ref();
    }

    void operator=(Ref&& other)
    {
        if (m_value == other.m_value)
        {
            return;
        }

        unref();

        m_value = other.m_value;
        m_references = other.m_references;

        other.m_value = nullptr;
        other.m_references = nullptr;
    }

    T *operator->()
    {
        ERR_COND(is_null(), "Trying to dereference a null pointer");
        return m_value;
    }

    const T *operator->() const
    {
        ERR_COND(is_null(), "Trying to dereference a null pointer");
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

    operator bool() const
    {
        return !is_null();
    }

    template <typename Subclass>
    inline Ref<Subclass> cast_to() const
    {
        if (is_null())
        {
            return nullptr;
        }

        if (m_value->template is<Subclass>())
        {
            return Ref<Subclass>(static_cast<Subclass *>(m_value), m_references);
        }
        return nullptr;
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
        // if (is_null())
        //     return;

        // *m_references += 1;
    }

    void unref()
    {
        // if (is_null())
        //     return;

        // *m_references -= 1;

        // if (*m_references == 0)
        // {
        //     delete m_value;
        //     delete m_references;

        //     m_value = nullptr;
        //     m_references = nullptr;
        // }
    }
};

template <typename T, typename... Args>
inline Ref<T> make_ref(Args... args)
{
    return Ref<T>(new T(args...));
}
