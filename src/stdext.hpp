#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace stdext
{

struct string_hash
{
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(const char *str) const { return hash_type{}(str); }
    size_t operator()(std::string_view str) const { return hash_type{}(str); }
    size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

template <typename T>
using string_map = std::unordered_map<std::string, T, string_hash, std::equal_to<>>;

/// Version of `std::shared_ptr` with all reference counting done atomically.
template <typename T>
class atomic_ptr
{
public:
    atomic_ptr();
    atomic_ptr(std::nullptr_t);

    /// Manage an external pointer. Beyond this point this pointer must no be manually free'd.
    atomic_ptr(T *);

    atomic_ptr(const atomic_ptr&);
    atomic_ptr(atomic_ptr&&) noexcept;

    ~atomic_ptr();

    atomic_ptr& operator=(const atomic_ptr&);
    atomic_ptr& operator=(atomic_ptr&&) noexcept;

    T *operator->();
    const T *operator->() const;

    operator bool() const { return m_ptr != nullptr; }

private:
    T *m_ptr;
    std::atomic_size_t *m_reference_count;
};

template <typename T>
atomic_ptr<T>::atomic_ptr()
    : m_ptr(nullptr), m_reference_count(nullptr)
{
}

template <typename T>
atomic_ptr<T>::atomic_ptr(std::nullptr_t)
    : m_ptr(nullptr), m_reference_count(nullptr)
{
}

template <typename T>
atomic_ptr<T>::atomic_ptr(T *ptr)
    : m_ptr(ptr), m_reference_count(new std::atomic_size_t(1))
{
}

template <typename T>
atomic_ptr<T>::atomic_ptr(const atomic_ptr& other)
    : m_ptr(other.m_ptr), m_reference_count(other.m_reference_count)
{
    if (other.m_reference_count)
        other.m_reference_count->fetch_add(1);
}

template <typename T>
atomic_ptr<T>::atomic_ptr(atomic_ptr&& other) noexcept
    : m_ptr(other.m_ptr), m_reference_count(other.m_reference_count)
{
}

template <typename T>
atomic_ptr<T>::~atomic_ptr()
{
    if (m_reference_count && m_reference_count->fetch_sub(1) == 1)
    {
        delete m_ptr;
        delete m_reference_count;

        m_ptr = nullptr;
        m_reference_count = nullptr;
    }
}

template <typename T>
atomic_ptr<T>& atomic_ptr<T>::operator=(const atomic_ptr& other)
{
    this->~atomic_ptr();

    if (other.m_ptr != nullptr)
    {
        m_ptr = other.m_ptr;
        m_reference_count = other.m_reference_count;
        m_reference_count->fetch_add(1);
    }
    return *this;
}

template <typename T>
atomic_ptr<T>& atomic_ptr<T>::operator=(atomic_ptr&& other) noexcept
{
    this->~atomic_ptr();

    m_ptr = other.m_ptr;
    m_reference_count = other.m_reference_count;
    return *this;
}

template <typename T>
T *atomic_ptr<T>::operator->()
{
    if (m_ptr == nullptr)
        std::abort();
    return this->m_ptr;
}

template <typename T>
const T *atomic_ptr<T>::operator->() const
{
    if (m_ptr == nullptr)
        std::abort();
    return this->m_ptr;
}

/// Allocate an atomically reference-counted smart pointer.
template <typename T, typename... Args>
inline atomic_ptr<T> make_atomic(Args&&...args)
{
    return atomic_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace stdext
