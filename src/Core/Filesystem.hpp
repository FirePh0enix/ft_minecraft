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

enum class FileKind
{
    Packed,
};

struct File
{
    File();

    ALWAYS_INLINE FileKind kind() const
    {
        return m_kind;
    }

    std::vector<char> read_to_buffer() const;
    String read_to_string() const;

private:
    friend class Filesystem;

    FileKind m_kind;
    size_t m_size;

    union
    {
        size_t m_offset;
        int m_fd;
    };
};

class Filesystem
{
public:
    static void open_data();

    /**
     * Returns the path of the executable (ex: `/usr/bin/ft_minecraft`).
     */
    static std::filesystem::path current_executable_path();

    /**
     * Returns the directory where is located the executable (ex: `/usr/bin`).
     */
    static std::filesystem::path current_executable_directory();

    static std::string resolve_absolute(const std::string& path);

    static Result<File> open_file(const StringView& path);

    static void read_raw(const File& file, void *buffer, size_t size);

private:
    static inline DataPack s_data_pack;
};
