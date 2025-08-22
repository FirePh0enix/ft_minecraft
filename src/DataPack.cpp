#include "DataPack.hpp"

#include <iostream>

DataPack::DataPack()
{
}

void DataPack::open(const std::filesystem::path& path)
{
    m_stream.open(path, std::ifstream::out | std::ifstream::binary);
    assert(m_stream.is_open() && "Cannot open file for writing");
}

void DataPack::load_from_file(const std::string& path)
{
    m_stream.open(path, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);

    m_file_size = m_stream.tellg();
    m_stream.seekg(0);
}

void DataPack::add_file_to_data_pack(std::string_view path, const std::vector<char>& buffer)
{
    if (!m_file_initialized)
    {
        DataPackHeader header{};
        header.version = data_pack_version;

        m_stream.write((char *)&header, sizeof(DataPackHeader));
        m_file_initialized = true;

        size_t padding = sector_size - sizeof(DataPackHeader);

        while (padding--)
        {
            m_stream << '\0';
        }
    }

    DataPackFileHeader file_header{};
    file_header.size = buffer.size();
    memcpy(file_header.path, path.data(), path.size());
    memset(file_header.path + path.size(), '\0', sizeof(file_header.path) - path.size());

    m_stream.write((char *)&file_header, sizeof(DataPackFileHeader));

    size_t size_padded = ((buffer.size() - 1) / sector_size + 1) * sector_size;
    size_t padding = size_padded - buffer.size();

    m_stream.write(buffer.data(), (std::streamsize)buffer.size());

    while (padding--)
    {
        m_stream << '\0';
    }
}

Result<std::vector<char>> DataPack::read_file(std::string_view path)
{
    size_t cursor = sector_size;
    m_stream.seekg(sector_size);

    while (cursor < m_file_size)
    {
        DataPackFileHeader file_header{};
        m_stream.read((char *)&file_header, sizeof(DataPackFileHeader));

        if (std::string_view(file_header.path) == path)
        {
            std::vector<char> buffer(file_header.size);
            m_stream.read(buffer.data(), file_header.size);
            return buffer;
        }
        else
        {
            size_t size_padded = ((file_header.size - 1) / sector_size + 1) * sector_size;
            cursor += sector_size + size_padded;
            m_stream.seekg((std::fstream::off_type)size_padded, std::fstream::cur);
        }
    }

    return Error(ErrorKind::FileNotFound);
}
