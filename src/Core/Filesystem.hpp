#pragma once

#include <filesystem>

#include "Core/Result.hpp"

static inline std::string get_data_directory()
{
    std::string path;

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
    path += std::filesystem::current_path().string() + "/.local/share";
#endif

    path += "/ft_minecraft";

    return path;
}

static inline std::string get_config_directory()
{
    std::string path;

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
    path += std::filesystem::current_path().string() + "/.config";
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
