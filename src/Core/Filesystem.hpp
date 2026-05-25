#pragma once

#include <filesystem>

#include "Core/IO.hpp"
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

class File;

class FileReader : public Reader
{
public:
    FileReader(const File *fp)
        : m_fp(fp)
    {
    }

    virtual Result<size_t> read_raw(void *buf, size_t size) override;
    virtual size_t size() override;
    virtual bool eof() override;

private:
    const File *m_fp;
    bool m_eof = false;
};

class FileWriter : public Writer
{
public:
    FileWriter(int fd)
        : m_fd(fd)
    {
    }

    virtual Result<size_t> write_raw(const void *buf, size_t size) override;

private:
    int m_fd;
};

class File
{
public:
    friend class FileReader;
    friend class FileWriter;

    File();
    void close();

    FileReader reader() const { return FileReader(this); }
    FileWriter writer() const { return FileWriter(m_fd); }

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
