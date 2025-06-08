#pragma once

#include <expected>
#include <print>

#ifndef STACKTRACE_SIZE
#define STACKTRACE_SIZE 16
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
};

template <>
struct std::formatter<ErrorKind> : std::formatter<std::string>
{
    auto format(const ErrorKind& kind, std::format_context& ctx) const
    {
        const char *msg;

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
        }

        return std::format_to(ctx.out(), "{}", msg);
    }
};

#ifdef __has_vulkan

static inline const char *string_vk_result(VkResult input_value)
{
    switch (input_value)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_NOT_PERMITTED_KHR:
        return "VK_ERROR_NOT_PERMITTED";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
        return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
        return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
    case VK_PIPELINE_BINARY_MISSING_KHR:
        return "VK_PIPELINE_BINARY_MISSING_KHR";
    case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
        return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
    default:
        return "Unhandled VkResult";
    }
}

#endif

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

#ifdef __has_vulkan

#ifdef __DEBUG__

    Error(vk::Result result, StackTrace stacktrace = StackTrace::current())
        : m_stacktrace(stacktrace)
    {
        m_kind = kind_from_vk_result(result);
        m_vk_result = result;
    }

#else

    Error(vk::Result result)
    {
        m_kind = kind_from_vk_result(result);
        m_vk_result = result;
    }

#endif // __DEBUG__

    static ErrorKind kind_from_vk_result(vk::Result result)
    {
        ErrorKind kind;

        switch (result)
        {
        case vk::Result::eErrorOutOfDeviceMemory:
            kind = ErrorKind::OutOfDeviceMemory;
            break;
        case vk::Result::eErrorOutOfHostMemory:
            kind = ErrorKind::OutOfMemory;
            break;
        default:
            kind = ErrorKind::BadDriver;
            break;
        }

        return kind;
    }

#endif // __has_vulkan

private:
    ErrorKind m_kind;

    union
    {
        int m_errno_value;

#ifdef __has_vulkan
        vk::Result m_vk_result = vk::Result::eErrorUnknown;
#endif
    };

#ifdef __DEBUG__
    StackTrace m_stacktrace;
#endif
};

#define YEET(EXPECTED)                                \
    do                                                \
    {                                                 \
        auto __result = EXPECTED;                     \
        if (!__result.has_value())                    \
            return std::unexpected(__result.error()); \
    } while (0)

#define EXPECT(EXPECTED)              \
    do                                \
    {                                 \
        auto __result = EXPECTED;     \
        if (!__result.has_value())    \
        {                             \
            __result.error().print(); \
            exit(1);                  \
        }                             \
    } while (0)

#ifdef __has_vulkan

#define YEET_RESULT_E(RESULT)                 \
    do                                        \
    {                                         \
        auto __result = RESULT;               \
        if (__result != vk::Result::eSuccess) \
            return std::unexpected(__result); \
    } while (0)

#define YEET_RESULT(RESULT)                          \
    do                                               \
    {                                                \
        auto __result = RESULT;                      \
        if (__result.result != vk::Result::eSuccess) \
            return std::unexpected(__result.result); \
    } while (0)

#endif

// #ifdef __DEBUG__

#define ERR_COND(COND, MESSAGE)                                            \
    do                                                                     \
    {                                                                      \
        if (COND)                                                          \
            std::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
    } while (0)

#define ERR_COND_R(COND, MESSAGE)                                          \
    do                                                                     \
    {                                                                      \
        if (COND)                                                          \
        {                                                                  \
            std::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            return;                                                        \
        }                                                                  \
    } while (0)

#define ERR_COND_V(COND, MESSAGE, ...)                        \
    do                                                        \
    {                                                         \
        if (COND)                                             \
        {                                                     \
            std::print("error: {}:{}: ", __FILE__, __LINE__); \
            std::println(MESSAGE, __VA_ARGS__);               \
        }                                                     \
    } while (0)

#define ERR_COND_VR(COND, MESSAGE, ...)                       \
    do                                                        \
    {                                                         \
        if (COND)                                             \
        {                                                     \
            std::print("error: {}:{}: ", __FILE__, __LINE__); \
            std::println(MESSAGE, __VA_ARGS__);               \
            return;                                           \
        }                                                     \
    } while (0)

#define ERR_EXPECT_R(EXPECTED, MESSAGE)                                    \
    do                                                                     \
    {                                                                      \
        auto __result = EXPECTED;                                          \
        if (!__result.has_value())                                         \
        {                                                                  \
            std::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
            return;                                                        \
        }                                                                  \
    } while (0)

#define ERR_EXPECT_B(EXPECTED, MESSAGE)                                \
    auto __result = EXPECTED;                                          \
    if (!__result.has_value())                                         \
    {                                                                  \
        std::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
        break;                                                         \
    }

#define ERR_EXPECT_C(EXPECTED, MESSAGE)                                \
    auto __result = EXPECTED;                                          \
    if (!__result.has_value())                                         \
    {                                                                  \
        std::println("error: {}:{}: {}", __FILE__, __LINE__, MESSAGE); \
        continue;                                                      \
    }

#ifdef __has_vulkan

#define ERR_RESULT_RET(RESULT)                                                                                 \
    do                                                                                                         \
    {                                                                                                          \
        auto __result = RESULT;                                                                                \
        if (__result.result != vk::Result::eSuccess)                                                           \
        {                                                                                                      \
            std::println("error: {}:{}: {}", __FILE__, __LINE__, string_vk_result((VkResult)__result.result)); \
            return;                                                                                            \
        }                                                                                                      \
    } while (0)

#define ERR_RESULT_E_RET(RESULT)                                                                        \
    do                                                                                                  \
    {                                                                                                   \
        auto __result = RESULT;                                                                         \
        if (__result != vk::Result::eSuccess)                                                           \
        {                                                                                               \
            std::println("error: {}:{}: {}", __FILE__, __LINE__, string_vk_result((VkResult)__result)); \
            return;                                                                                     \
        }                                                                                               \
    } while (0)

#endif

// #else

// #define ERR_COND(COND, MESSAGE)
// #define ERR_COND_V(COND, MESSAGE, ...)
// #define ERR_COND_VR(COND, MESSAGE, ...)
// #define ERR_EXPECT_R(EXPECTED, MESSAGE)
// #define ERR_EXPECT_B(EXPECTED, MESSAGE)
// #define ERR_EXPECT_C(EXPECTED, MESSAGE)

// #ifdef __has_vulkan
// #define ERR_RESULT_RET(RESULT)
// #define ERR_RESULT_E_RET(RESULT)
// #endif

// #endif

template <typename T>
using Expected = std::expected<T, Error>;

void initialize_error_handling(const char *filename);

/**
 * Modern C++ version of C's assert.
 */
#ifdef __DEBUG__

template <typename... Args>
inline void assert_error(bool condition, std::format_string<Args...> format, Args... args)
{
    if (!condition)
    {
        std::println(stderr, format, args...);
        std::abort();
    }
}

#else

template <typename... Args>
inline void assert_error(bool condition, std::format_string<Args...> format, Args... args)
{
}

#endif
