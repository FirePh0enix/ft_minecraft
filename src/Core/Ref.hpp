#pragma once

#include <cstdint>
#include <type_traits>

#include "Core/Alloc.hpp"
#include "Core/Definitions.hpp"

void ref_check_null_access(bool cond, const char *class_name);

template <typename T>
class Ref
{
public:
    using ReferenceType = uint32_t;

    ALWAYS_INLINE Ref()
        : m_ptr(nullptr), m_references(nullptr)
    {
    }

    ALWAYS_INLINE Ref(std::nullptr_t)
        : m_ptr(nullptr), m_references(nullptr)
    {
    }

    ALWAYS_INLINE explicit Ref(T *ptr)
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
            ref();
    }

    ~Ref()
    {
        if (!is_null())
            unref();
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

    inline T *operator->()
    {
        ref_check_null_access(is_null(), T::get_static_class_name());
        return m_ptr;
    }

    inline const T *operator->() const
    {
        ref_check_null_access(is_null(), T::get_static_class_name());
        return m_ptr;
    }

    inline T& operator*()
    {
        ref_check_null_access(is_null(), T::get_static_class_name());
        return *m_ptr;
    }

    inline const T& operator*() const
    {
        ref_check_null_access(is_null(), T::get_static_class_name());
        return *m_ptr;
    }

    ALWAYS_INLINE bool operator==(const Ref& other) const
    {
        return m_ptr == other.m_ptr;
    }

    ALWAYS_INLINE bool operator!=(const Ref& other) const
    {
        return m_ptr != other.m_ptr;
    }

    ALWAYS_INLINE bool operator<(const Ref& other) const
    {
        return m_ptr < other.m_ptr;
    }

    ALWAYS_INLINE operator bool() const
    {
        return !is_null();
    }

    template <typename B, typename = std::is_base_of<B, T>>
    ALWAYS_INLINE operator Ref<B>() const
    {
        return cast_to<B>();
    }

    ALWAYS_INLINE ReferenceType references()
    {
        return m_references ? *m_references : 0;
    }

    ALWAYS_INLINE bool is_null() const
    {
        return m_ptr == nullptr;
    }

    ALWAYS_INLINE T *ptr() const
    {
        return m_ptr;
    }

    template <typename B>
    ALWAYS_INLINE Ref<B> unchecked_cast_to() const
    {
        return Ref<B>(static_cast<B *>((B *)m_ptr), m_references);
    }

    template <typename B>
    Ref<B> cast_to() const
    {
        if (!is_null() && m_ptr->template is<B>())
        {
            return Ref<B>(static_cast<B *>((B *)m_ptr), m_references);
        }
        return nullptr;
    }

private:
    T *m_ptr;
    ReferenceType *m_references;

    ALWAYS_INLINE void ref()
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

#define newobj(T, ...) Ref<T>(alloc<T>(__VA_ARGS__))
