#pragma once

#include "Core/Error.hpp"

template <typename T>
class Ref
{
public:
    using ReferenceType = uint32_t;

    Ref()
        : m_ptr(nullptr), m_references(nullptr)
    {
    }

    Ref(std::nullptr_t)
        : m_ptr(nullptr), m_references(nullptr)
    {
    }

    explicit Ref(T *ptr)
        : m_ptr(ptr), m_references(new ReferenceType(1))
    {
    }

    Ref(const Ref& other)
    {
        if (!other.is_null())
        {
            m_ptr = other.m_ptr;
            m_references = other.m_references;

            ref();
        }
        else
        {
            m_ptr = nullptr;
            m_references = nullptr;
        }
    }

    Ref(T *ptr, ReferenceType *references)
        : m_ptr(ptr), m_references(references)
    {
        if (!is_null())
        {
            ref();
        }
    }

    ~Ref()
    {
        if (!is_null())
        {
            unref();
        }
    }

    Ref& operator=(std::nullptr_t)
    {
        if (!is_null())
        {
            unref();
        }

        m_ptr = nullptr;
        m_references = nullptr;

        return *this;
    }

    Ref& operator=(const Ref& other)
    {
        if (other.m_ptr == m_ptr)
        {
            return *this;
        }

        if (!is_null())
        {
            unref();
        }

        if (!other.is_null())
        {
            m_ptr = other.m_ptr;
            m_references = other.m_references;

            ref();
        }
        else
        {
            m_ptr = nullptr;
            m_references = nullptr;
        }

        return *this;
    }

    T *operator->()
    {
        ERR_COND_V(is_null(), "Trying to access a null ref of type {}", T::get_static_class_name());
        return m_ptr;
    }

    const T *operator->() const
    {
        ERR_COND_V(is_null(), "Trying to access a null ref of type {}", T::get_static_class_name());
        return m_ptr;
    }

    T& operator*()
    {
        ERR_COND_V(is_null(), "Trying to dereference a null ref of type {}", T::get_static_class_name());
        return *m_ptr;
    }

    const T& operator*() const
    {
        ERR_COND_V(is_null(), "Trying to dereference a null ref of type {}", T::get_static_class_name());
        return *m_ptr;
    }

    bool operator==(const Ref& other) const
    {
        return m_ptr == other.m_ptr;
    }

    bool operator!=(const Ref& other) const
    {
        return m_ptr != other.m_ptr;
    }

    operator bool() const
    {
        return !is_null();
    }

    template <typename B, typename = std::is_base_of<B, T>>
    operator Ref<B>() const
    {
        return cast_to<B>();
    }

    inline ReferenceType references()
    {
        return m_references ? *m_references : 0;
    }

    inline bool is_null() const
    {
        return m_ptr == nullptr;
    }

    inline T *ptr() const
    {
        return m_ptr;
    }

    template <typename B>
    Ref<B> cast_to() const
    {
        if (is_null())
        {
            return nullptr;
        }

        if (m_ptr->template is<B>())
        {
            return Ref<B>(static_cast<B *>((B *)m_ptr), m_references);
        }
        return nullptr;
    }

private:
    T *m_ptr;
    ReferenceType *m_references;

    void ref()
    {
        *m_references += 1;
    }

    void unref()
    {
        if (--*m_references == 0)
        {
            delete m_ptr;
            delete m_references;

            m_ptr = nullptr;
            m_references = nullptr;
        }
    }
};

template <typename T, typename... Args>
inline Ref<T> make_ref(Args... args)
{
    return Ref<T>(new T(args...));
}
