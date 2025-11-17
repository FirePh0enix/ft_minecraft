#include "Core/Filesystem.hpp"

#ifdef __platform_macos
#include <mach-o/dyld.h>
#endif

Result<int> Filesystem::init()
{
    s_data_pack.load_from_file(current_executable_directory().string() + "/ft_minecraft.data");
    return 0;
}

std::filesystem::path Filesystem::current_executable_path()
{
#ifdef __platform_linux
    return std::filesystem::canonical("/proc/self/exe");
#elif defined(__platform_windows)
    return GetModuleFileName(nullptr);
#elif defined(__platform_macos)

    char buf[PATH_MAX];
    uint32_t buf_size = PATH_MAX;
    if (_NSGetExecutablePath(buf, &buf_size) == 0)
    {
        std::string s;
        s.append(buf, buf_size);
        return std::filesystem::path(s);
    }

    return "";
#elif defined(__platform_web)
    return "/";
#endif
}

std::filesystem::path Filesystem::current_executable_directory()
{
    return current_executable_path().parent_path();
}

Result<std::string> Filesystem::read_file_to_string(const std::filesystem::path& path)
{
    ASSERT(s_data_pack.is_open(), "");
    Result<std::vector<char>> data_result = s_data_pack.read_file(path.string());
    YEET(data_result);
    std::vector<char> data = data_result.value();
    std::string text;
    text.append(data.data(), data.size());
    return text;
}

Result<std::vector<char>> Filesystem::read_file_to_buffer(const std::filesystem::path& path)
{
    ASSERT(s_data_pack.is_open(), "");
    Result<std::vector<char>> data_result = s_data_pack.read_file(path.string());
    YEET(data_result);
    return data_result.value();
}
