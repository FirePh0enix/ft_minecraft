#include "Core/Filesystem.hpp"
#include "Core/Result.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __platform_macos
#include <mach-o/dyld.h>
#endif

File::File()
    : m_fd(-1)
{
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
    int fd = open(path.data(), O_RDONLY);

    if (fd == -1)
        return Error(ErrorKind::FileNotFound);

    struct stat st{};
    stat(path.data(), &st);

    File file;
    file.m_fd = fd;
    file.m_size = st.st_size;

    return file;
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
    read(file.m_fd, buffer, size);
}
