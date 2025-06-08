#pragma once

#include <print>

enum class LogLevel : uint8_t
{
    Info = 0,
    Warn = 1,
    Error = 2,
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
    }
}

template <typename T, typename... Args>
inline void log(LogLevel level, std::format_string<Args...> fmt, Args&&...args)
{
    std::print("{} ", log_level_str(level));
    std::print(fmt, args...);
}

template <typename T, typename... Args>
inline void info(std::format_string<Args...> fmt, Args&&...args)
{
    log(LogLevel::Info, fmt, args...);
}

template <typename T, typename... Args>
inline void warn(std::format_string<Args...> fmt, Args&&...args)
{
    log(LogLevel::Warn, fmt, args...);
}

template <typename T, typename... Args>
inline void error(std::format_string<Args...> fmt, Args&&...args)
{
    log(LogLevel::Error, fmt, args...);
}
