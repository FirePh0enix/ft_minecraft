#include "Core/ZLib.hpp"

#include <zlib.h>

#include <limits>

#define DEFLATE_BUFFER_SIZE (4 * 1024)
#define INFLATE_BUFFER_SIZE (4 * 1024)

Result<void> ZLib::deflate(std::span<const std::byte> data, std::vector<uint8_t>& compressed_data)
{
    if (data.size_bytes() > std::numeric_limits<uInt>::max())
        return Error(ErrorKind::ReadFailure);

    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm{};
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
        return Error(ErrorKind::ReadFailure);

    struct StreamGuard { z_stream *stream; ~StreamGuard() { deflateEnd(stream); } } guard{&strm};

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
    return Result<void>();
}

Result<void> ZLib::deflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& compressed_data)
{
    if (data.size_bytes() > std::numeric_limits<uInt>::max())
        return Error(ErrorKind::ReadFailure);

    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm{};
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
        return Error(ErrorKind::ReadFailure);

    struct StreamGuard { z_stream *stream; ~StreamGuard() { deflateEnd(stream); } } guard{&strm};

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
    return Result<void>();
}

Result<void> ZLib::inflate(std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data)
{
    if (data.empty() || data.size_bytes() > std::numeric_limits<uInt>::max())
        return Error(ErrorKind::ReadFailure);

    uint8_t tmp[INFLATE_BUFFER_SIZE];

    z_stream strm{};
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = INFLATE_BUFFER_SIZE;
    if (inflateInit(&strm) != Z_OK)
        return Error(ErrorKind::ReadFailure);

    struct StreamGuard { z_stream *stream; ~StreamGuard() { inflateEnd(stream); } } guard{&strm};

    int res = Z_OK;
    while (res != Z_STREAM_END)
    {
        res = ::inflate(&strm, Z_NO_FLUSH);
        if (res != Z_OK && res != Z_STREAM_END)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = INFLATE_BUFFER_SIZE;
        }

        if (res == Z_OK && strm.avail_in == 0)
            return Error(ErrorKind::ReadFailure);
    }

    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE - strm.avail_out);
    return Result<void>();
}

Result<void> ZLib::inflate_with_cancellation(std::stop_token token, std::span<const std::byte> data, std::vector<uint8_t>& uncompressed_data)
{
    if (data.empty() || data.size_bytes() > std::numeric_limits<uInt>::max())
        return Error(ErrorKind::ReadFailure);

    uint8_t tmp[INFLATE_BUFFER_SIZE];

    z_stream strm{};
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.next_in = (uint8_t *)data.data();
    strm.avail_in = data.size_bytes();
    strm.next_out = tmp;
    strm.avail_out = INFLATE_BUFFER_SIZE;
    if (inflateInit(&strm) != Z_OK)
        return Error(ErrorKind::ReadFailure);

    struct StreamGuard { z_stream *stream; ~StreamGuard() { inflateEnd(stream); } } guard{&strm};

    int res = Z_OK;
    while (res != Z_STREAM_END)
    {
        if (token.stop_requested())
            return Error(ErrorKind::Cancelled);

        res = ::inflate(&strm, Z_NO_FLUSH);
        if (res != Z_OK && res != Z_STREAM_END)
            return Error(ErrorKind::ReadFailure);

        if (strm.avail_out == 0)
        {
            uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE);
            strm.next_out = tmp;
            strm.avail_out = INFLATE_BUFFER_SIZE;
        }

        if (res == Z_OK && strm.avail_in == 0)
            return Error(ErrorKind::ReadFailure);
    }

    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE - strm.avail_out);
    return Result<void>();
}
