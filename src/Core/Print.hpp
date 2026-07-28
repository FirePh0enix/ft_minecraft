#pragma once

#include <cstdio>
#include <format>

template <typename... _Args>
void print(FILE *fp, std::format_string<_Args...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <>
inline void print(FILE *fp, std::format_string<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fflush(fp);
}

template <typename... _Args>
void print(std::format_string<_Args...> fmt, _Args&&...args)
{
    print(stdout, fmt, std::forward<_Args>(args)...);
}

template <>
inline void print(std::format_string<> fmt)
{
    print(stdout, fmt);
}

template <typename... _Args>
void println(FILE *fp, std::format_string<_Args...> fmt, _Args&&...args)
{
    std::string s = format(fmt, std::forward<_Args>(args)...);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <>
inline void println(FILE *fp, std::format_string<> fmt)
{
    std::string s = format(fmt);
    fwrite(s.data(), 1, s.size(), fp);
    fputc('\n', fp);
    fflush(fp);
}

template <typename... _Args>
void println(std::format_string<_Args...> fmt, _Args&&...args)
{
    println(stdout, fmt, std::forward<_Args>(args)...);
}

template <typename... _Args>
void println(std::format_string<> fmt)
{
    println(stdout, fmt);
}
