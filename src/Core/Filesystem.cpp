#include "Core/Filesystem.hpp"

Result<int> Filesystem::init()
{
    s_data_pack.load_from_file(current_executable_path().string() + "/ft_minecraft.data");
    return 0;
}

std::filesystem::path Filesystem::current_executable_path()
{
#ifdef __platform_linux
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#elif defined(__platform_windows)
#error "FIXME"
    return GetModuleFileName(nullptr);
#elif defined(__platform_macos)

    char buf[PATH_MAX];
    uint32_t buf_size = PATH_MAX;
    if (_NSGetExecutablePath(buf, &buf_size) == 0)
    {
        std::string s;
        s.resize(buf_size);
        s.append(buf, buf_size);
        return s;
    }

    return "";
#endif
}

Result<std::string> Filesystem::read_file_to_string(const std::filesystem::path& path)
{
    Result<std::vector<char>> data_result = s_data_pack.read_file(path.string());
    YEET(data_result);
    std::vector<char> data = data_result.value();
    std::string text;
    text.append(data.data(), data.size());
    return text;
}

Result<std::vector<char>> Filesystem::read_file_to_buffer(const std::filesystem::path& path)
{
    Result<std::vector<char>> data_result = s_data_pack.read_file(path.string());
    YEET(data_result);
    return data_result.value();
}
