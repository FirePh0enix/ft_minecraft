#pragma once

#include "Core/Format.hpp"

template <typename... _Args>
void print(FILE *fp, FormatString<_Args...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <>
inline void print(FILE *fp, BasicFormatString<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <typename... _Args>
void print(FormatString<_Args...> fmt, _Args&&...args)
{
    print(stdout, fmt, std::forward<_Args>(args)...);
}

template <>
inline void print(BasicFormatString<> fmt)
{
    print(stdout, fmt);
}

template <typename... _Args>
void println(FILE *fp, FormatString<_Args...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <>
inline void println(FILE *fp, FormatString<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <typename... _Args>
void println(FormatString<_Args...> fmt, _Args&&...args)
{
    println(stdout, fmt, std::forward<_Args>(args)...);
}

template <typename... _Args>
void println(FormatString<> fmt)
{
    println(stdout, fmt);
}
