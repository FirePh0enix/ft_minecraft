#include "Render/Driver.hpp"

Ref<RenderingDriver> RenderingDriver::singleton = nullptr;

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
    case TextureFormat::D32:
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

Expected<Ref<Buffer>> RenderingDriver::create_buffer_from_data(size_t size, Span<uint8_t> data, BufferUsage flags, BufferVisibility visibility)
{
    auto buffer_result = create_buffer(size, flags, visibility);
    YEET(buffer_result);

    Ref<Buffer> buffer = buffer_result.value();
    buffer->update(data, 0);

    return buffer;
}

Shader Shader::create(const std::string& name)
{
    // TODO: Add support for compute shaders.

#ifdef __platform_web
    std::string filename = std::format("../assets/shaders/wgsl/{}.wgsl", name);

    Shader::Ref ref;
    ref.filename = filename;
    ref.stages.push_back(Stage{.kind = ShaderKind::Vertex, .entry = "vertex_main"});
    ref.stages.push_back(Stage{.kind = ShaderKind::Fragment, .entry = "fragment_main"});

    Shader shader;
    shader.m_refs.push_back(ref);

    return shader;
#else
    std::string vertex_filename = std::format("assets/shaders/{}.vert.spv", name);
    std::string fragment_filename = std::format("assets/shaders/{}.frag.spv", name);

    Shader::Ref vertex_ref{};
    vertex_ref.filename = vertex_filename;
    vertex_ref.stages.push_back(Stage{.kind = ShaderKind::Vertex, .entry = "main"});

    Shader::Ref fragment_ref{};
    fragment_ref.filename = fragment_filename;
    fragment_ref.stages.push_back(Stage{.kind = ShaderKind::Fragment, .entry = "main"});

    Shader shader;
    shader.m_refs.push_back(vertex_ref);
    shader.m_refs.push_back(fragment_ref);

    return shader;
#endif
}
