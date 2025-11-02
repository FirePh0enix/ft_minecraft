#pragma once

template <typename T>
struct Tag
{
    using Type = T;
};

template <typename T, typename... Ts>
struct IndexOf;

template <typename T, typename... Ts>
struct IndexOf<T, T, Ts...> : std::integral_constant<std::size_t, 0>
{
};

template <typename T, typename U, typename... Ts>
struct IndexOf<T, U, Ts...> : std::integral_constant<std::size_t, 1 + IndexOf<T, Ts...>::value>
{
};
