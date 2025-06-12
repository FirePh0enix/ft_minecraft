#pragma once

#include "Core/Format.hpp"

#include <print>

enum class LogLevel : uint8_t
{
    Info = 0,
    Warn = 1,
    Error = 2,
    Debug = 3,
};

inline const char *log_level_str(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:
        return "(info )";
    case LogLevel::Warn:
        return "(warn )";
    case LogLevel::Error:
        return "(error)";
    case LogLevel::Debug:
        return "(debug)";
    }
    return "";
}

template <class... Args>
inline void log_msg(LogLevel level, std::format_string<Args...> fmt, Args&&...args)
{
    std::print("{} ", log_level_str(level));
    std::println(fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void info(std::format_string<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void warn(std::format_string<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Warn, fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void error(std::format_string<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <class... Args>
inline void debug(std::format_string<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
