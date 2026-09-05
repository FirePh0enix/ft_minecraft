#pragma once

#include <filesystem>

#include "Core/IO.hpp"

class File;

class FileReader : public Reader
{
public:
    FileReader(const File *fp)
        : m_fp(fp)
    {
    }

    virtual std::expected<size_t, Error> read_raw(void *buf, size_t size) override;
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

    virtual std::expected<size_t, Error> write_raw(const void *buf, size_t size) override;

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

    static std::string get_data_directory();

    static bool exists(std::string_view path);

    static std::expected<File, Error> open_file(std::string_view path, bool rw = false);
    static std::expected<void, Error> make_dirs(std::string_view path);
};
