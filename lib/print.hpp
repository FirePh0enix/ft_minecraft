#pragma once

#include "format.hpp"

namespace lib
{

template <typename... _Args>
void print(FILE *fp, format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <>
inline void print(FILE *fp, format_string<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <typename... _Args>
void print(format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    print(stdout, fmt, std::forward<_Args>(args)...);
}

template <>
inline void print(format_string<> fmt)
{
    print(stdout, fmt);
}

template <typename... _Args>
void println(FILE *fp, format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <>
inline void println(FILE *fp, format_string<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <typename... _Args>
void println(format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    println(stdout, fmt, std::forward<_Args>(args)...);
}

template <typename... _Args>
void println(format_string<> fmt)
{
    println(stdout, fmt);
}

} // namespace lib
