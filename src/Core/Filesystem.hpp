#pragma once

#include <filesystem>

#include "Core/Result.hpp"

static inline String get_data_directory()
{
    String path;

#ifdef __platform_linux
    if (char *data_home = std::getenv("XDG_DATA_HOME"))
    {
        path += data_home;
    }
    else
    {
        path += std::getenv("HOME");
        path += "/.local/share";
    }
#else
    path += std::getenv("HOME");
    path += "/.local/share";
#endif

    path += "/ft_minecraft";

    return path;
}

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

    std::vector<char> read_to_buffer() const;
    String read_to_string() const;

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

    static Result<File> open_file(const StringView& path);

    static void read_raw(const File& file, void *buffer, size_t size);
};
