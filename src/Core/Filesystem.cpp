#include "Core/Filesystem.hpp"

#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#ifdef __platform_windows
#include <io.h>
#include <limits>
#include <vector>
#else
#include <unistd.h>
#endif

#ifdef __platform_macos
#include <mach-o/dyld.h>
#endif

#ifdef __platform_windows
#include <windows.h>
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
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;)
    {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
            throw std::filesystem::filesystem_error("GetModuleFileNameW",
                std::error_code(GetLastError(), std::system_category()));
        if (length < buffer.size())
            return std::filesystem::path(std::wstring(buffer.data(), length));
        buffer.resize(buffer.size() * 2);
    }
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

std::string Filesystem::get_data_directory()
{
    return "./appdata/";
}

bool Filesystem::exists(std::string_view path)
{
#ifdef __platform_windows
    struct _stat64 s{};
    return _wstat64(std::filesystem::path(path).c_str(), &s) != -1;
#else
    struct stat s{};
    return stat(path.data(), &s) != -1;
#endif
}

std::expected<File, Error> Filesystem::open_file(std::string_view path, bool rw)
{
#ifdef __platform_windows
    int fd = _wopen(std::filesystem::path(path).c_str(),
        (rw ? (_O_RDWR | _O_CREAT) : _O_RDONLY) | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
    int fd = open(path.data(), rw ? (O_RDWR | O_CREAT) : O_RDONLY, S_IRUSR | S_IWUSR);
#endif

    if (fd == -1)
    {
        // Capture errno before logging or querying the working directory.
        Error failure(ErrorKind::FileNotFound);
        std::error_code ec;
        const auto cwd = std::filesystem::current_path(ec);
        error("Failed to open file '{}' (working directory: '{}')", path, cwd.string());
        return std::unexpected(failure);
    }

#ifdef __platform_windows
    struct _stat64 st{};
    if (_fstat64(fd, &st) == -1)
    {
        _close(fd);
        return std::unexpected(Error(ErrorKind::ReadFailure));
    }
#else
    struct stat st{};
    stat(path.data(), &st);
#endif

    File file;
    file.m_fd = fd;
    file.m_size = st.st_size;

    return file;
}

std::expected<void, Error> Filesystem::make_dirs(std::string_view path)
{
    std::filesystem::create_directories(path.data());
    return std::expected<void, Error>();
}

void File::close()
{
#ifdef __platform_windows
    _close(m_fd);
#else
    ::close(m_fd);
#endif
    m_fd = -1;
}

std::expected<size_t, Error> FileReader::read_raw(void *buffer, size_t size)
{
#ifdef __platform_windows
    // The CRT accepts an unsigned count but returns a signed int.
    if (size > static_cast<size_t>(std::numeric_limits<int>::max()))
        return std::unexpected(Error(ErrorKind::ReadFailure));
    int r = _read(m_fp->m_fd, buffer, static_cast<unsigned int>(size));
#else
    ssize_t r = ::read(m_fp->m_fd, buffer, size);
#endif
    if (r == 0)
    {
        m_eof = true;
        return 0;
    }
    if (r < 0 || static_cast<size_t>(r) < size)
        return std::unexpected(Error(ErrorKind::ReadFailure));
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

std::expected<size_t, Error> FileWriter::write_raw(const void *buffer, size_t len)
{
#ifdef __platform_windows
    if (len > static_cast<size_t>(std::numeric_limits<int>::max()))
        return std::unexpected(Error(ErrorKind::WriteFailure));
    int r = _write(m_fd, buffer, static_cast<unsigned int>(len));
#else
    ssize_t r = write(m_fd, buffer, len);
#endif
    if (r == -1)
        return std::unexpected(Error(ErrorKind::WriteFailure));
    return (size_t)r;
}
