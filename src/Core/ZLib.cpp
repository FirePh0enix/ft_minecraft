#include "Core/ZLib.hpp"

#include <zlib.h>

#define DEFLATE_BUFFER_SIZE (4 * 1024)
#define INFLATE_BUFFER_SIZE (4 * 1024)

Result<void> ZLib::deflate(std::span<const std::byte> data, std::vector<uint8_t>& compressed_data)
{
    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    deflateInit(&strm, Z_BEST_COMPRESSION);

    while (strm.avail_in != 0)
    {
        int res = ::deflate(&strm, Z_NO_FLUSH);

        if (res != Z_OK)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = DEFLATE_BUFFER_SIZE;
        }
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK)
    {
        if (strm.avail_out == 0)
        {
            compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = DEFLATE_BUFFER_SIZE;
        }
        deflate_res = ::deflate(&strm, Z_FINISH);
    }

    if (deflate_res != Z_STREAM_END)
        return Error(ErrorKind::ReadFailure);

    compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE - strm.avail_out);
    deflateEnd(&strm);

    return Result<void>();
}

Result<void> ZLib::deflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& compressed_data)
{
    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    deflateInit(&strm, Z_BEST_COMPRESSION);

    while (strm.avail_in != 0)
    {
        if (token.stop_requested())
            return Error(ErrorKind::Cancelled);

        int res = ::deflate(&strm, Z_NO_FLUSH);

        if (res != Z_OK)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = DEFLATE_BUFFER_SIZE;
        }
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK)
    {
        if (token.stop_requested())
            return Error(ErrorKind::Cancelled);

        if (strm.avail_out == 0)
        {
            compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = DEFLATE_BUFFER_SIZE;
        }
        deflate_res = ::deflate(&strm, Z_FINISH);
    }

    if (deflate_res != Z_STREAM_END)
        return Error(ErrorKind::ReadFailure);

    compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE - strm.avail_out);
    deflateEnd(&strm);

    return Result<void>();
}

Result<void> ZLib::inflate(std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data)
{
    uint8_t tmp[INFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = INFLATE_BUFFER_SIZE;
    inflateInit(&strm);

    while (strm.avail_in != 0)
    {
        int res = ::inflate(&strm, Z_NO_FLUSH);
        if (res < 0)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = INFLATE_BUFFER_SIZE;
        }
    }

    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE - strm.avail_out);
    inflateEnd(&strm);

    return Result<void>();
}

Result<void> inflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data)
{
    uint8_t tmp[INFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = INFLATE_BUFFER_SIZE;
    inflateInit(&strm);

    while (strm.avail_in != 0)
    {
        if (token.stop_requested())
            return Error(ErrorKind::Cancelled);

        int res = ::inflate(&strm, Z_NO_FLUSH);
        if (res < 0)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = INFLATE_BUFFER_SIZE;
        }
    }

    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE - strm.avail_out);
    inflateEnd(&strm);

    return Result<void>();
}
