#pragma once

#include "Core/Class.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

static constexpr size_t sector_size = 512;
static constexpr uint32_t data_pack_version = 1;

// TODO: Replace the full path by some kind of file allocation table.
// TODO: Add compression

struct DataPackHeader
{
    uint32_t version = data_pack_version;
};

struct DataPackFileHeader
{
    uint32_t size;
    char path[508];
};

class DataPack : public Object
{
    CLASS(DataPack, Object);

public:
    DataPack();

    void open(const std::filesystem::path& path);
    void load_from_file(const std::string& path);

    void add_file_to_data_pack(std::string_view path, const std::vector<char>& buffer);
    std::vector<char> read_file(std::string_view path);

private:
    std::fstream m_stream;
    size_t m_file_size = 0;
    bool m_file_initialized = false;
};
