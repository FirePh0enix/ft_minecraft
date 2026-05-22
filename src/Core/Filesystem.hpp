#pragma once

#include <filesystem>

#include "Core/Containers/LocalVector.hpp"
#include "Core/Result.hpp"

static inline String get_config_directory()
{
    String path;

#ifdef __platform_linux
    if (char *data_home = std::getenv("XDG_CONFIG_HOME"))
    {
        path += data_home;
    }
    else
    {
        path += std::getenv("HOME");
        path += "/.config";
    }
#else
    path += std::getenv("HOME");
    path += "/.config";
#endif

    path += "/ft_minecraft";

    return path;
}

struct File
{
    File();
    void close();

    Result<void> read_raw(void *buffer, size_t size) const;
    Result<void> read_to_buffer(LocalVector<char>& buffer) const;
    Result<String> read_to_string() const;

    Result<void> write_raw(const void *buffer, size_t len);

private:
    friend class Filesystem;

    size_t m_size;
    int m_fd;
};

class Filesystem
{
public:
    /**
     * Returns the path of the executable (ex: `/usr/bin/ft_minecraft`).
     */
    static std::filesystem::path current_executable_path();

    /**
     * Returns the directory where is located the executable (ex: `/usr/bin`).
     */
    static std::filesystem::path current_executable_directory();

    static String get_data_directory();

    static bool exists(const StringView& path);

    static Result<File> open_file(const StringView& path, bool rw = false);
    static Result<void> make_dirs(const StringView& path);

    static inline std::optional<String> data_dir;
};
