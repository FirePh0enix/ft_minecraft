#pragma once

#ifndef __platform_linux
#include <filesystem>
#endif

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
