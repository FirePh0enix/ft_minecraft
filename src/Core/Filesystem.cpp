#include "Core/Filesystem.hpp"
#include "Core/Result.hpp"

#ifdef __platform_macos
#include <mach-o/dyld.h>
#endif

File::File()
    : m_kind(FileKind::Packed), m_offset(0)
{
}

void Filesystem::open_data()
{
    s_data_pack.load_from_file(current_executable_directory().string() + "/ft_minecraft.data");
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

Result<File> Filesystem::open_file(const StringView& path)
{
    if (path.starts_with("assets://"))
    {
        if (!s_data_pack.is_open())
            return Error(ErrorKind::FileNotFound);

        const StringView& path2 = path.slice(9);

        File file;
        file.m_kind = FileKind::Packed;

        Result<DataPackFileInfo> result = s_data_pack.find_file(path2);
        YEET(result);

        DataPackFileInfo info = result.value();
        file.m_offset = info.offset;
        file.m_size = info.size;

        return file;
    }
    else
    {
        // TODO: Allow opening files.
        return Error(ErrorKind::FileNotFound);
    }
}

std::vector<char> File::read_to_buffer() const
{
    std::vector<char> buffer(m_size);
    Filesystem::read_raw(*this, buffer.data(), m_size);
    return buffer;
}

String File::read_to_string() const
{
    String str;
    str.resize(m_size);

    Filesystem::read_raw(*this, str.data(), m_size);
    return str;
}

void Filesystem::read_raw(const File& file, void *buffer, size_t size)
{
    EXPECT(s_data_pack.read_file(file.m_offset, buffer, size));
}

std::string Filesystem::resolve_absolute(const std::string& path)
{
    if (path.starts_with("assets://"))
        return "assets/" + path.substr(9);
    return path;
}
