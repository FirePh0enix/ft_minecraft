#include "Render/Driver.hpp"

size_t size_of(const TextureFormat& format)
{
    switch (format)
    {
    case TextureFormat::R8Unorm:
        return 1;
    case TextureFormat::R32Sfloat:
    case TextureFormat::RGBA8Srgb:
    case TextureFormat::RGBA8Unorm:
    case TextureFormat::BGRA8Srgb:
    case TextureFormat::Depth32:
        return 4;
    case TextureFormat::RG32Sfloat:
        return 8;
    case TextureFormat::RGB32Sfloat:
        return 12;
    case TextureFormat::RGBA32Sfloat:
        return 16;
    }

    return 0;
}

size_t size_of(const IndexType& format)
{
    switch (format)
    {
    case IndexType::Uint16:
        return 2;
    case IndexType::Uint32:
        return 4;
    };

    return 0;
}

Result<Ref<Buffer>> RenderingDriver::create_buffer_from_data(size_t size, View<uint8_t> data, BufferUsage flags, BufferVisibility visibility)
{
    auto buffer_result = create_buffer(size, flags, visibility);
    YEET(buffer_result);

    Ref<Buffer> buffer = buffer_result.value();
    buffer->update(data, 0);

    return buffer;
}
