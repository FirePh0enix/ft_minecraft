#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Result.hpp"

#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#ifdef __platform_macos
#include <mach-o/dyld.h>
#endif

File::File()
    : m_size(0), m_fd(-1)
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

    return "./";
#elif defined(__platform_web)
    return "./";
#endif
}

std::filesystem::path Filesystem::current_executable_directory()
{
    return current_executable_path().parent_path();
}

String Filesystem::get_data_directory()
{
    if (data_dir.has_value())
        return data_dir.value();

#ifdef __platform_linux
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home != nullptr)
        return format("{}/ft_minecraft/", xdg_data_home);

    const char *home = getenv("HOME");
    if (home != nullptr)
        return format("{}/.local/share/ft_minecraft");

    return format("./appdata/");
#else
    return format("./appdata/");
#endif
}

bool Filesystem::exists(const StringView& path)
{
    struct stat s{};
    return stat(path.data(), &s) != -1;
}

Result<File> Filesystem::open_file(const StringView& path, bool rw)
{
    int fd = open(path.data(), rw ? (O_RDWR | O_CREAT) : O_RDONLY, S_IRUSR | S_IWUSR);

    if (fd == -1)
        return Error(ErrorKind::FileNotFound);

    struct stat st{};
    stat(path.data(), &st);

    File file;
    file.m_fd = fd;
    file.m_size = st.st_size;

    return file;
}

Result<void> Filesystem::make_dirs(const StringView& path)
{
    // TODO: replace with C lib
    std::filesystem::create_directories(path.data());
    return Result<void>();
}

void File::close()
{
    ::close(m_fd);
    m_fd = -1;
}

Result<size_t> FileReader::read_raw(void *buffer, size_t size)
{
    ssize_t r = ::read(m_fp->m_fd, buffer, size);
    if (r == 0)
    {
        m_eof = true;
        return 0;
    }
    if (r < (ssize_t)size)
        return Error(ErrorKind::ReadFailure);
    return (size_t)r;
}

size_t FileReader::size()
{
    return m_fp->m_size;
}

bool FileReader::eof()
{
    return m_eof;
}

Result<size_t> FileWriter::write_raw(const void *buffer, size_t len)
{
    ssize_t r = write(m_fd, buffer, len);
    if (r == -1)
        return Error(ErrorKind::WriteFailure);
    return (size_t)r;
}
