#pragma once

#include <filesystem>

#include "Core/Result.hpp"
#include "DataPack.hpp"

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

class Filesystem
{
public:
    static Result<int> init();

    /**
     * Returns the path of the executable (ex: `/usr/bin/ft_minecraft`).
     */
    static std::filesystem::path current_executable_path();

    /**
     * Returns the directory where is located the executable (ex: `/usr/bin`).
     */
    static std::filesystem::path current_executable_directory();

    static Result<std::string> read_file_to_string(const std::filesystem::path& path);
    static Result<std::vector<char>> read_file_to_buffer(const std::filesystem::path& path);

private:
    static inline DataPack s_data_pack;
};
