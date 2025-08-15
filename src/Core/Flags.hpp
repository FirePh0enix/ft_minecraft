#pragma once

#include <concepts>
#include <type_traits>

template <typename T, typename = std::is_enum<T>>
struct Flags
{
public:
    using IntType = std::underlying_type_t<T>;

    Flags()
        : m_value((T)0)
    {
    }

    Flags(T value)
        : m_value(value)
    {
    }

    inline operator T()
    {
        return m_value;
    }

    inline operator IntType()
    {
        return (IntType)m_value;
    }

    inline constexpr std::strong_ordering operator<=>(Flags<T> rhs) const
    {
        return (typename Flags<T>::IntType)(m_value) <=> (typename Flags<T>::IntType)rhs.m_value;
    }

    inline constexpr Flags<T> operator|(Flags<T> rhs) const
    {
        return Flags<T>((T)((typename Flags<T>::IntType)(m_value) | (typename Flags<T>::IntType)rhs));
    }

    inline constexpr Flags<T> operator|(T rhs) const
    {
        return Flags<T>((T)((typename Flags<T>::IntType)(m_value) | (typename Flags<T>::IntType)rhs));
    }

    inline constexpr Flags<T> operator&(Flags<T> rhs) const
    {
        return Flags<T>((T)((typename Flags<T>::IntType)(m_value) | (typename Flags<T>::IntType)rhs));
    }

    inline constexpr Flags<T> operator&(T rhs) const
    {
        return Flags<T>((T)((typename Flags<T>::IntType)(m_value) | (typename Flags<T>::IntType)rhs));
    }

    inline constexpr bool has_all(const Flags<T>& other) const
    {
        return ((IntType)m_value & (IntType)other.m_value) == (IntType)other.m_value;
    }

    inline constexpr bool has_any(const Flags<T>& other) const
    {
        return ((IntType)m_value & (IntType)other.m_value) != (IntType)0;
    }

private:
    T m_value;
};

template <typename BitType>
struct FlagTraits;

#define DEFINE_FLAG_TRAITS(NAME)                  \
    template <>                                   \
    struct FlagTraits<NAME>                       \
    {                                             \
        static constexpr bool placeholder = true; \
    };

template <typename BitType, typename = decltype(FlagTraits<BitType>())>
inline constexpr Flags<BitType> operator|(BitType lhs, BitType rhs)
{
    return Flags<BitType>(lhs).operator|(rhs);
}

template <typename BitType, typename = decltype(FlagTraits<BitType>())>
inline constexpr Flags<BitType> operator&(BitType lhs, BitType rhs)
{
    return Flags<BitType>(lhs).operator&(rhs);
}
