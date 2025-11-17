#pragma once

#include "Core/Format.hpp"
#include "Core/Print.hpp"

#include <cstdint>

enum class LogLevel : uint8_t
{
    Info = 0,
    Warn = 1,
    Error = 2,
    Debug = 3,
};

static inline const char *log_level_strs[] = {
    "(info )",
    "(warn )",
    "(error)",
    "(debug)",
};

inline const char *log_level_str(LogLevel level)
{
    return log_level_strs[(size_t)level];
}

template <typename... Args>
inline void log_msg(LogLevel level, FormatString<Args...> fmt, Args&&...args)
{
    print("{} ", log_level_str(level));
    println(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(FormatString<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(FormatString<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(FormatString<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(FormatString<Args...> fmt, Args&&...args)
{
    log_msg(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}
