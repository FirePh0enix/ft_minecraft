#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Core/Logger.hpp"

#ifndef STACKTRACE_SIZE
#define STACKTRACE_SIZE 128
#endif

struct StackTrace
{
    struct Frame
    {
        const char *function;
        const char *filename;
        uint32_t line;
    };

    bool stop_at_main = true;

    std::array<Frame, STACKTRACE_SIZE> frames;
    size_t length = 0;
    size_t total_length = 0;
    bool non_exhaustive = false;
    bool early_end = false;

    static const StackTrace& current();

    void print(FILE *fp = stderr, size_t skip_frame = 0) const;
};

enum class ErrorKind : uint16_t
{
    /**
     * @brief A generic error to indicate something went wrong.
     */
    Unknown = 0x0,

    /**
     * @brief The CPU ran out of memory.
     */
    OutOfMemory = 0x1,

    /**
     * @brief A file is missing.
     */
    FileNotFound = 0x2,

    /**
     * @brief A generic error to indicate something went wrong while talking to the GPU.
     */
    BadDriver = 0x1000,

    /**
     * @brief The GPU run out of memory.
     */
    OutOfDeviceMemory = 0x1001,

    /**
     * @brief No GPU can be use by the engine.
     */
    NoSuitableDevice = 0x1002,

    /**
     * @brief A shader failed to compile.
     */
    ShaderCompilationFailed = 0x1003,

    /**
     * @brief ImGui initialization failed.
     */
    ImGuiInitFailed = 0x1004,
};

template <>
struct Formatter<ErrorKind> : public FormatterBase
{
    void format(const ErrorKind& kind, FormatContext& ctx) const
    {
        const char *msg = "";

        switch (kind)
        {
        case ErrorKind::Unknown:
            msg = "Unknown";
            break;
        case ErrorKind::OutOfMemory:
            msg = "Out of memory";
            break;
        case ErrorKind::FileNotFound:
            msg = "File not found";
            break;
        case ErrorKind::BadDriver:
            msg = "Bad driver";
            break;
        case ErrorKind::OutOfDeviceMemory:
            msg = "Out of GPU memory";
            break;
        case ErrorKind::NoSuitableDevice:
            msg = "No suitable device";
            break;
        case ErrorKind::ShaderCompilationFailed:
            msg = "Shader compilation failed";
            break;
        case ErrorKind::ImGuiInitFailed:
            msg = "ImGui initialization failed";
            break;
        }

        ctx.write_str(msg, (std::streamsize)std::strlen(msg));
    }
};

class Error
{
public:
#ifdef __DEBUG__

    Error(ErrorKind kind, StackTrace stacktrace = StackTrace::current())
        : m_kind(kind), m_stacktrace(stacktrace)
    {
        m_errno_value = errno;

        if (errno != 0)
            errno = 0;
    }

#else

    Error(ErrorKind kind)
        : m_kind(kind)
    {
    }

#endif

    void print(FILE *fp = stderr);

    bool is_other() const
    {
        return (uint32_t)m_kind < 0x1000;
    }

    bool is_graphics() const
    {
        return (uint32_t)m_kind >= 0x1000 && (uint32_t)m_kind < 0x2000;
    }

private:
    ErrorKind m_kind;

    union
    {
        int m_errno_value;
    };

#ifdef __DEBUG__
    StackTrace m_stacktrace;
#endif
};

#define ERR_COND(COND, MESSAGE)                                \
    do                                                         \
    {                                                          \
        if (COND)                                              \
            ::error("{}:{}: {}", __FILE__, __LINE__, MESSAGE); \
    } while (0)

#define ERR_COND_R(COND, MESSAGE, ...)                         \
    do                                                         \
    {                                                          \
        if (COND)                                              \
        {                                                      \
            ::error("{}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            return __VA_ARGS__;                                \
        }                                                      \
    } while (0)

#define ERR_COND_B(COND, MESSAGE)                              \
    do                                                         \
    {                                                          \
        if (COND)                                              \
        {                                                      \
            ::error("{}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            break;                                             \
        }                                                      \
    } while (0)

#define ERR_COND_BV(COND, MESSAGE, ...)                                     \
    do                                                                      \
    {                                                                       \
        if (COND)                                                           \
        {                                                                   \
            ::error("{}:{}: {}", __FILE__, __LINE__, MESSAGE, __VA_ARGS__); \
            break;                                                          \
        }                                                                   \
    } while (0)

#define ERR_COND_C(COND, MESSAGE, ...)                         \
    do                                                         \
    {                                                          \
        if (COND)                                              \
        {                                                      \
            ::error("{}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            continue;                                          \
        }                                                      \
    } while (0)

#define ERR_COND_V(COND, MESSAGE, ...)                     \
    do                                                     \
    {                                                      \
        if (COND)                                          \
        {                                                  \
            ::print("error: {}:{}: ", __FILE__, __LINE__); \
            ::println(MESSAGE, __VA_ARGS__);               \
        }                                                  \
    } while (0)

#define ERR_COND_VR(COND, MESSAGE, ...)                    \
    do                                                     \
    {                                                      \
        if (COND)                                          \
        {                                                  \
            ::print("error: {}:{}: ", __FILE__, __LINE__); \
            ::println(MESSAGE, __VA_ARGS__);               \
            return;                                        \
        }                                                  \
    } while (0)

#define ERR_COND_VRV(COND, RETURN_VALUE, MESSAGE, ...)     \
    do                                                     \
    {                                                      \
        if (COND)                                          \
        {                                                  \
            ::print("error: {}:{}: ", __FILE__, __LINE__); \
            ::println(MESSAGE, __VA_ARGS__);               \
            return RETURN_VALUE;                           \
        }                                                  \
    } while (0)

#define ERR_EXPECT_R(EXPECTED, MESSAGE)                                 \
    do                                                                  \
    {                                                                   \
        auto __result = EXPECTED;                                       \
        if (!__result.has_value())                                      \
        {                                                               \
            ::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            return;                                                     \
        }                                                               \
    } while (0)

#define ERR_EXPECT_VR(EXPECTED, RET, MESSAGE, ...)         \
    do                                                     \
    {                                                      \
        auto __result = EXPECTED;                          \
        if (!__result.has_value())                         \
        {                                                  \
            ::print("error: {}:{}: ", __FILE__, __LINE__); \
            ::println(MESSAGE, __VA_ARGS__);               \
            return RET;                                    \
        }                                                  \
    } while (0)

#define ERR_EXPECT_B(EXPECTED, MESSAGE)                             \
    auto __result = EXPECTED;                                       \
    if (!__result.has_value())                                      \
    {                                                               \
        ::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
        break;                                                      \
    }

#define ERR_EXPECT_C(EXPECTED, MESSAGE)                             \
    auto __result = EXPECTED;                                       \
    if (!__result.has_value())                                      \
    {                                                               \
        ::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
        continue;                                                   \
    }

void initialize_error_handling(const char *filename);
