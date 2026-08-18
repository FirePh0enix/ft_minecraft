#include "Render/Renderer.hpp"

#include "Core/Assert.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Math.hpp"
#include "Core/Result.hpp"
#include "Core/Stacktrace.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Profiler.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>

#include <format>
#include <mutex>
#include <random>
#include <span>

// clang-format off
static const uint32_t missing_texture_data[16 * 16]{
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
};
// clang-format on

size_t size_of(const WGPUTextureFormat& format)
{
    switch (format)
    {
    case WGPUTextureFormat_R8Unorm:
        return 1;
    case WGPUTextureFormat_R32Float:
    case WGPUTextureFormat_RGBA8UnormSrgb:
    case WGPUTextureFormat_RGBA8Unorm:
    case WGPUTextureFormat_BGRA8UnormSrgb:
    case WGPUTextureFormat_Depth32Float:
        return 4;
    case WGPUTextureFormat_RG32Float:
        return 8;
    case WGPUTextureFormat_RGBA32Float:
        return 16;
    default:
        ERR_COND(false, "invalid texture format");
    }

    return 0;
}

size_t size_of(const WGPUIndexFormat& format)
{
    switch (format)
    {
    case WGPUIndexFormat_Uint16:
        return 2;
    case WGPUIndexFormat_Uint32:
        return 4;
    default:
        return 0;
    };
}

size_t size_of(const WGPUVertexFormat& format)
{
    switch (format)
    {
    case WGPUVertexFormat_Float32:
        return 1 * sizeof(float);
    case WGPUVertexFormat_Float32x2:
        return 2 * sizeof(float);
    case WGPUVertexFormat_Float32x3:
        return 3 * sizeof(float);
    case WGPUVertexFormat_Float32x4:
        return 4 * sizeof(float);
    default:
        return 0;
    };
}

Buffer::~Buffer()
{
    wgpuBufferRelease(m_buffer);
    Renderer::get().m_device_memory_freed += m_size;
}

Result<std::shared_ptr<Buffer>> Buffer::create(size_t size, WGPUBufferUsage usage, BufferVisibility visibility)
{
    WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    desc.size = size;
    desc.usage = usage;

    // WebGPU requires the size of an uniform buffer to a multiple of 16 bytes.
    if (usage & WGPUBufferUsage_Uniform && size % 16 != 0)
    {
        desc.size = (((size - 1) / 16) + 1) * 16;
        size = desc.size;
    }

    WGPUBuffer wgpu_buffer = ({
        std::lock_guard<std::mutex> guard(Renderer::get().get_device_mutex());
        wgpuDeviceCreateBuffer(Renderer::get().m_device, &desc);
    });
    ERR_COND_VRV(wgpu_buffer == nullptr, Error(ErrorKind::OutOfDeviceMemory), "Failed to create buffer of size {}", size);

    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    buffer->m_buffer = wgpu_buffer;
    buffer->m_usage = usage;
    buffer->m_size = size;
    buffer->m_visibility = visibility;

    if (visibility == BufferVisibility::GPUAndCPU)
    {
        ASSERT_V(usage & WGPUBufferUsage_CopySrc, "Buffer must be copiable");
        desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        buffer->m_transfer_buffer = ({
            std::lock_guard<std::mutex> guard(Renderer::get().get_device_mutex());
            wgpuDeviceCreateBuffer(Renderer::get().m_device, &desc);
        });
    }

    Renderer::get().m_device_memory_allocated += size;

    return buffer;
}

void Buffer::update(std::span<const std::byte> view, size_t offset)
{
    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueWriteBuffer(Renderer::get().m_queue, m_buffer, static_cast<uint64_t>(offset), view.data(), view.size());
}

Texture::~Texture()
{
    if (!m_external)
    {
        wgpuTextureRelease(m_texture);
    }
}

Result<std::shared_ptr<Texture>> Texture::create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage, WGPUTextureDimension dimension, uint32_t layers, uint32_t mip_level)
{
    WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    desc.usage = usage;
    desc.dimension = dimension;
    desc.size = WGPUExtent3D{.width = width, .height = height, .depthOrArrayLayers = layers == 0 ? 1 : layers};
    desc.format = format;
    desc.mipLevelCount = mip_level;

    if (mip_level > 1)
    {
        desc.usage |= WGPUTextureUsage_StorageBinding;
    }

    WGPUTexture texture = wgpuDeviceCreateTexture(Renderer::get().m_device, &desc);
    if (!texture)
        return Error(ErrorKind::OutOfDeviceMemory);

    std::shared_ptr<Texture> tex = std::make_shared<Texture>();
    tex->m_texture = texture;
    tex->m_width = width;
    tex->m_height = height;
    tex->m_layers = layers;
    tex->m_mip_level = mip_level;
    tex->m_format = format;

    return tex;
}

std::shared_ptr<Texture> Texture::create_from_handle(WGPUTexture texture)
{
    std::shared_ptr<Texture> tex = std::make_shared<Texture>();
    tex->m_texture = texture;
    tex->m_width = wgpuTextureGetWidth(texture);
    tex->m_height = wgpuTextureGetHeight(texture);
    tex->m_layers = wgpuTextureGetDepthOrArrayLayers(texture);
    tex->m_mip_level = wgpuTextureGetMipLevelCount(texture);
    tex->m_format = wgpuTextureGetFormat(texture);
    tex->m_external = true;
    return tex;
}

Result<std::shared_ptr<Texture>> Texture::load(std::string_view path)
{
    File file = TRY(Filesystem::open_file(path));

    std::vector<char> buffer;
    TRY(file.reader().read_to_buffer(buffer));
    file.close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);
    if (data == nullptr)
        return Error(ErrorKind::ReadFailure);

    std::shared_ptr<Texture> texture = TRY(Texture::create(w, h, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureDimension_2D, 1, 1));
    texture->update(std::span((std::byte *)data, w * h * 4));

    stbi_image_free(data);

    return texture;
}

void Texture::update(std::span<const std::byte> view, uint32_t layer)
{
    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());

#ifndef __platform_web
    WGPUTexelCopyTextureInfo copy_info = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    copy_info.texture = m_texture;
    copy_info.aspect = WGPUTextureAspect_All;
    copy_info.origin.z = layer;

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = size_of(m_format) * m_width;
    layout.rowsPerImage = m_height;

    WGPUExtent3D write_size{.width = m_width, .height = m_height, .depthOrArrayLayers = 1};

    wgpuQueueWriteTexture(Renderer::get().m_queue, &copy_info, view.data(), view.size(), &layout, &write_size);
#else
    WGPUImageCopyTexture copy_info{};
    copy_info.texture = texture;
    copy_info.aspect = WGPUTextureAspect_All;
    copy_info.origin.x = 0;
    copy_info.origin.y = 0;
    copy_info.origin.z = layer;
    copy_info.mipLevel = 0;

    WGPUTextureDataLayout layout{};
    layout.bytesPerRow = size_of(format) * m_width;
    layout.rowsPerImage = m_height;
    layout.offset = 0;

    WGPUExtent3D write_size(m_width, m_height, 1);

    wgpuQueueWriteTexture(Renderer::get().m_queue, &copy_info, view.data(), view.size(), &layout, &write_size);
#endif
}

Result<WGPUTextureView> Texture::get_view(WGPUTextureViewDimension dimension, WGPUTextureAspect aspect, int base_layer, int layer_count)
{
    if (layer_count == -1)
        layer_count = (int)m_layers;

    ViewDesc desc{};
    desc.dimension = dimension;
    desc.aspect = aspect;
    desc.base_layer = base_layer;
    desc.layer_count = layer_count;

    auto iter = m_views.find(desc);
    if (iter != m_views.end())
        return iter->second;

    WGPUTextureViewDescriptor view_desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    view_desc.format = WGPUTextureFormat_Undefined;
    view_desc.dimension = dimension;
    view_desc.mipLevelCount = m_mip_level;
    view_desc.baseArrayLayer = base_layer;
    view_desc.arrayLayerCount = layer_count;
    view_desc.aspect = aspect;

    WGPUTextureView view = wgpuTextureCreateView(m_texture, &view_desc);
    if (!view)
        return Error(ErrorKind::OutOfDeviceMemory);

    m_views[desc] = view;
    return view;
}

Result<std::shared_ptr<Mesh>> Mesh::create_from_data(std::span<const std::byte> indices, std::span<const glm::vec3> positions, std::span<const glm::vec3> normals, std::span<const std::byte> uvs, WGPUIndexFormat index_type, WGPUVertexFormat uv_format)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    std::shared_ptr<Buffer> index_buffer = TRY(Buffer::create(indices.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index));
    index_buffer->update(indices);

    std::shared_ptr<Buffer> vertex_buffer = TRY(Buffer::create(positions.size() * sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    vertex_buffer->update(std::as_bytes(positions));

    std::shared_ptr<Buffer> uv_buffer = TRY(Buffer::create(uvs.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    uv_buffer->update(std::as_bytes(uvs));

    std::shared_ptr<Buffer> normal_buffer;
    if (normals.size() > 0)
    {
        normal_buffer = TRY(Buffer::create(normals.size() * sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
        normal_buffer->update(std::as_bytes(normals));
    }

    return std::make_shared<Mesh>(vertex_count, index_type, uv_format, index_buffer, vertex_buffer, normal_buffer, uv_buffer);
}

static WGPUShaderModule create_shader_module(const std::shared_ptr<Shader>& shader)
{
    WGPUShaderModuleDescriptor module_desc{};
    module_desc.label = WGPU_STRING_VIEW_INIT;

#ifndef __platform_web
    WGPUShaderSourceWGSL wgsl_source = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl_source.code = {.data = shader->get_source_string().data(), .length = shader->get_source_string().size()};
    module_desc.nextInChain = &wgsl_source.chain;
#else
    const View<char> shader_code = shader->get_code();

    WGPUShaderModuleWGSLDescriptor wgsl_source{};
    wgsl_source.chain = {.next = nullptr, .sType = WGPUSType_ShaderModuleWGSLDescriptor};
    wgsl_source.code = shader_code.data();
    module_desc.nextInChain = &wgsl_source.chain;
#endif

    return wgpuDeviceCreateShaderModule(Renderer::get().device(), &module_desc);
}

Material::~Material()
{
    for (const auto& [_, pipeline] : m_pipelines)
        wgpuRenderPipelineRelease(pipeline);
}

std::shared_ptr<Material> Material::create(const std::shared_ptr<Shader>& shader, MaterialFlags flags, WGPUCullMode cull_mode, WGPUVertexFormat uv_format, Instance instance, WGPUPrimitiveTopology topology)
{
    std::shared_ptr<Material> material = std::make_shared<Material>();
    material->m_shader = shader;
    material->m_flags = flags;
    material->m_cull_mode = cull_mode;
    material->m_uv_format = uv_format;
    material->m_attributes = instance.attribs;
    material->m_instance_stride = instance.stride;
    material->m_topology = topology;
    return material;
}

WGPURenderPipeline Material::get_pipeline(const RenderPass& pass)
{
    PipelineKey key(pass.textures, pass.depth);
    auto p = m_pipelines.find(key);
    if (p != m_pipelines.end())
        return p->second;

    WGPURenderPipeline pipeline = create_pipeline(pass);
    m_pipelines[key] = pipeline;
    return pipeline;
}

WGPURenderPipeline Material::create_pipeline(const RenderPass& pass)
{
    std::vector<WGPUVertexBufferLayout> buffers;
    buffers.reserve(3 + m_attributes.size());

    uint32_t attrib_index = 0;

    WGPUVertexAttribute vertex_attrib{};
    if (!m_flags.has_all(MaterialFlagBits::NoPosition))
    {
        vertex_attrib.format = WGPUVertexFormat_Float32x3;
        vertex_attrib.offset = 0;
        vertex_attrib.shaderLocation = attrib_index++;
        buffers.push_back(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib});
    }

    WGPUVertexAttribute normal_attrib{};
    if (!m_flags.has_any(MaterialFlagBits::NoNormal))
    {
        normal_attrib.format = WGPUVertexFormat_Float32x3;
        normal_attrib.offset = 0;
        normal_attrib.shaderLocation = attrib_index++;
        buffers.push_back(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib});
    }

    WGPUVertexAttribute uv_attrib{};
    if (!m_flags.has_any(MaterialFlagBits::NoUV))
    {
        uv_attrib.format = m_uv_format;
        uv_attrib.offset = 0;
        uv_attrib.shaderLocation = attrib_index++;
        buffers.push_back(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = size_of(m_uv_format), .attributeCount = 1, .attributes = &uv_attrib});
    }

    std::vector<WGPUVertexAttribute> attributes;
    attributes.reserve(m_attributes.size());
    for (uint32_t i = 0; i < m_attributes.size(); i++)
    {
        InstanceAttribute attrib = m_attributes[i];
        attributes.push_back(WGPUVertexAttribute{.nextInChain = nullptr, .format = attrib.format, .offset = attrib.offset, .shaderLocation = attrib_index + i});
    }
    if (m_attributes.size() > 0)
    {
        buffers.push_back(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Instance, .arrayStride = m_instance_stride, .attributeCount = attributes.size(), .attributes = attributes.data()});
    }

    WGPUShaderModule module = create_shader_module(m_shader);
    ERR_COND_R(module == nullptr, "Unable to compile shader", nullptr);

    WGPUVertexState vertex_state = WGPU_VERTEX_STATE_INIT;
    vertex_state.buffers = buffers.data();
    vertex_state.bufferCount = buffers.size();
    vertex_state.entryPoint = WGPU_STRING_VIEW("vertex_main");
    vertex_state.module = module;

    WGPUBlendState blend_state{};

    if (!m_flags.has_any(MaterialFlagBits::Transparency))
    {
        blend_state.color.srcFactor = WGPUBlendFactor_One;
        blend_state.color.dstFactor = WGPUBlendFactor_Zero;
        blend_state.color.operation = WGPUBlendOperation_Add;

        blend_state.alpha.srcFactor = WGPUBlendFactor_One;
        blend_state.alpha.dstFactor = WGPUBlendFactor_Zero;
        blend_state.alpha.operation = WGPUBlendOperation_Add;
    }
    else
    {
        blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blend_state.color.operation = WGPUBlendOperation_Add;

        blend_state.alpha.srcFactor = WGPUBlendFactor_One;
        blend_state.alpha.dstFactor = WGPUBlendFactor_Zero;
        blend_state.alpha.operation = WGPUBlendOperation_Add;
    }

    std::vector<WGPUColorTargetState> color_states;
    color_states.reserve(pass.textures.size());
    for (const RenderTarget& target : pass.textures)
        color_states.push_back(WGPUColorTargetState{.nextInChain = nullptr, .format = target.format, .blend = target.blending ? &blend_state : nullptr, .writeMask = WGPUColorWriteMask_All});

    WGPUFragmentState fragment_state = WGPU_FRAGMENT_STATE_INIT;
    fragment_state.targets = color_states.data();
    fragment_state.targetCount = color_states.size();
    fragment_state.entryPoint = WGPU_STRING_VIEW("fragment_main");
    fragment_state.module = module;

    WGPUDepthStencilState depth_state = WGPU_DEPTH_STENCIL_STATE_INIT;

    if (pass.depth.has_value())
    {
        depth_state.format = pass.depth.value().format;
        depth_state.depthWriteEnabled = !m_flags.has_any(MaterialFlagBits::DisableDepthTest) ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depth_state.depthCompare = WGPUCompareFunction_LessEqual;

        if (m_flags.has_any(MaterialFlagBits::StencilMask))
        {
            depth_state.stencilFront.compare = WGPUCompareFunction_Always;
            depth_state.stencilFront.passOp = WGPUStencilOperation_Replace;

            depth_state.stencilBack.compare = WGPUCompareFunction_Always;
            depth_state.stencilBack.passOp = WGPUStencilOperation_Replace;
        }
        else if (m_flags.has_any(MaterialFlagBits::Stencil))
        {
            depth_state.stencilFront.compare = WGPUCompareFunction_Equal;
            depth_state.stencilFront.passOp = WGPUStencilOperation_Replace;
            depth_state.stencilFront.failOp = WGPUStencilOperation_Keep;

            depth_state.stencilBack.compare = WGPUCompareFunction_Equal;
            depth_state.stencilBack.passOp = WGPUStencilOperation_Replace;
            depth_state.stencilBack.failOp = WGPUStencilOperation_Keep;
        }
    }

    WGPUPrimitiveState primitive_state = WGPU_PRIMITIVE_STATE_INIT;
    primitive_state.cullMode = m_cull_mode;
    primitive_state.frontFace = WGPUFrontFace_CCW; // FIXME
    primitive_state.topology = m_topology;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.layout = m_shader->get_pipeline_layout();
    desc.primitive = primitive_state;
    desc.vertex = vertex_state;
    desc.fragment = color_states.size() == 0 ? nullptr : &fragment_state;
    desc.depthStencil = pass.depth.has_value() ? &depth_state : nullptr;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(Renderer::get().device(), &desc);
    ERR_COND_R(pipeline == nullptr, "Failed to compile the render pipeline", nullptr);

    wgpuShaderModuleRelease(module);

    return pipeline;
}

BindGroup::~BindGroup()
{
    if (m_bind_group)
        wgpuBindGroupRelease(m_bind_group);
}

std::shared_ptr<BindGroup> BindGroup::create(const std::shared_ptr<Shader>& shader)
{
    std::shared_ptr<BindGroup> bg = std::make_shared<BindGroup>();
    bg->m_shader = shader;
    return bg;
}

void BindGroup::set_param(std::string_view name, WGPUTextureView texture)
{
    std::optional<Binding> binding_result = m_shader->get_binding(name);
    ASSERT_V(texture != nullptr, "Texture specified for {} is null", name);
    ASSERT_V(binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    m_caches[std::string(name)] = MaterialParamCache{.kind = binding_result.value().kind, .texture = texture};
    m_dirty = true;
}

void BindGroup::set_param(std::string_view name, const std::shared_ptr<Buffer>& buffer, size_t offset, size_t size)
{
    std::optional<Binding> binding_result = m_shader->get_binding(name);
    ERR_COND_VR(buffer == nullptr, "Buffer specified for {} is null", name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    m_caches[std::string(name)] = MaterialParamCache{.kind = BindingKind::UniformBuffer, .buffer = buffer, .offset = offset, .size = size};
    m_dirty = true;
}

const MaterialParamCache& BindGroup::get_param(std::string_view name) const
{
    ASSERT_V(m_caches.contains(name), "Cache missing {}", name);
    return m_caches.find(name)->second;
}

WGPUBindGroup BindGroup::get_bind_group()
{
    if (m_dirty)
        create_bind_group();
    return m_bind_group;
}

void BindGroup::create_bind_group()
{
    if (m_bind_group != nullptr)
        wgpuBindGroupRelease(m_bind_group);

    std::vector<WGPUBindGroupEntry> entries;
    entries.reserve(m_shader->get_bindings().size());

    for (const auto& [name, binding] : m_shader->get_bindings())
    {
        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            const MaterialParamCache& cache = get_param(name);

            WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
            entry.binding = binding.binding;
            entry.textureView = cache.texture;

            entries.push_back(entry);

            SamplerDescriptor sampler = m_shader->get_sampler(name);

            WGPUSampler sampler_result = Renderer::get().get_sampler(sampler);
            ERR_COND_B(sampler_result == nullptr, "Unable to create a sampler");

            WGPUBindGroupEntry sampler_entry = WGPU_BIND_GROUP_ENTRY_INIT;
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.sampler = sampler_result;

            entries.push_back(sampler_entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            const MaterialParamCache& cache = get_param(name);
            ASSERT(cache.buffer->flags() & WGPUBufferUsage_Uniform, "Missing Uniform flag on buffer for param `{}`", name);

            WGPUBindGroupEntry entry = WGPU_BIND_GROUP_ENTRY_INIT;
            entry.binding = binding.binding;
            entry.buffer = cache.buffer->handle();
            entry.offset = cache.offset;
            entry.size = std::min(cache.buffer->size(), cache.size);

            entries.push_back(entry);
        }
        break;
        }
    }

    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = m_shader->get_bind_group_layout();
    desc.entries = entries.data();
    desc.entryCount = entries.size();

    m_bind_group = wgpuDeviceCreateBindGroup(Renderer::get().device(), &desc);
    ERR_COND_R(m_bind_group == nullptr, "Invalid bind group");

    m_dirty = false;
}

WGPUSampler SamplerCache::get(const SamplerDescriptor& desc)
{
    auto sampler_opt = m_samplers.find(desc);
    if (sampler_opt != m_samplers.end())
        return sampler_opt->second;

    WGPUSamplerDescriptor d = WGPU_SAMPLER_DESCRIPTOR_INIT;
    d.magFilter = desc.mag_filter;
    d.minFilter = desc.min_filter;
    d.addressModeU = desc.address_mode.u;
    d.addressModeV = desc.address_mode.v;
    d.addressModeW = desc.address_mode.w;
    d.compare = desc.compare;

    WGPUSampler sampler = wgpuDeviceCreateSampler(Renderer::get().device(), &d);
    m_samplers[desc] = sampler;

    return sampler;
}

SamplerCache::~SamplerCache()
{
    for (auto [_, sampler] : m_samplers)
        wgpuSamplerRelease(sampler);
}

Renderer::Renderer()
    : m_clouds_noise(0)
{
}

#ifndef __platform_web

WGPUAdapter request_adapter_sync(WGPUInstance instance)
{
    WGPUAdapter adapter;

    WGPURequestAdapterOptions adapter_options{};
    adapter_options.featureLevel = WGPUFeatureLevel_Core;

    WGPURequestAdapterCallbackInfo callback_info{
        .nextInChain = nullptr,
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *adapter_out, void *)
        {
            (void)status;
            (void)message;
            *((WGPUAdapter *)adapter_out) = adapter;
        },
        .userdata1 = (void *)&adapter,
        .userdata2 = nullptr,
    };
    WGPUFuture future = wgpuInstanceRequestAdapter(instance, &adapter_options, callback_info);
    // WGPUFutureWaitInfo info(future, false);
    // wgpuInstanceWaitAny(instance, 1, &info, 100000);
    (void)future;
    return adapter;
}

WGPUDevice request_device_sync(WGPUInstance instance, WGPUAdapter adapter, const WGPUDeviceDescriptor& options)
{
    (void)instance;
    WGPUDevice device;

    WGPURequestDeviceCallbackInfo callback_info{
        .nextInChain = nullptr,
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *device_out, void *)
        {
            (void)status;
            (void)message;
            *((WGPUDevice *)device_out) = device;
        },
        .userdata1 = (void *)&device,
        .userdata2 = nullptr,
    };
    WGPUFuture future = wgpuAdapterRequestDevice(adapter, &options, callback_info);
    // WGPUFutureWaitInfo info(future, false);
    // wgpuInstanceWaitAny(instance, 1, &info, 100000);
    (void)future;
    return device;
}

static WGPUSurface create_surface(WGPUInstance instance, SDL_Window *window)
{
    SDL_PropertiesID id = SDL_GetWindowProperties(window);
    WGPUSurfaceDescriptor surface_descriptor = {};
    WGPUSurface surface = {};
#if defined(__platform_macos)
    {
        auto m_metal_view = SDL_Metal_CreateView(window);

        WGPUSurfaceSourceMetalLayer surface_src_metal = {};
        surface_src_metal.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        surface_src_metal.layer = SDL_Metal_GetLayer(m_metal_view);
        surface_descriptor.nextInChain = &surface_src_metal.chain;
        surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    }
#elif defined(__platform_linux)
    const char *video_driver = SDL_GetCurrentVideoDriver();

    if (!video_driver)
        video_driver = "x11";

    if (SDL_strcmp(video_driver, "wayland") == 0)
    {
        void *w_display = SDL_GetPointerProperty(id, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        void *w_surface = SDL_GetPointerProperty(id, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        if (!w_display || !w_surface)
            return nullptr;
        WGPUSurfaceSourceWaylandSurface surface_src_wayland = {};
        surface_src_wayland.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        surface_src_wayland.display = w_display;
        surface_src_wayland.surface = w_surface;
        surface_descriptor.nextInChain = &surface_src_wayland.chain;
        surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    }
    else if (!SDL_strcmp(video_driver, "x11"))
    {
        void *x_display = SDL_GetPointerProperty(id, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        uint64_t x_window = SDL_GetNumberProperty(id, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (!x_display || !x_window)
            return nullptr;
        WGPUSurfaceSourceXlibWindow surface_src_xlib = {};
        surface_src_xlib.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        surface_src_xlib.display = x_display;
        surface_src_xlib.window = x_window;
        surface_descriptor.nextInChain = &surface_src_xlib.chain;
        surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    }
#elif defined(__platform_windows)
    {
        HWND hwnd = (HWND)SDL_GetPointerProperty(id, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (!hwnd)
            return nullptr;
        HINSTANCE hinstance = ::GetModuleHandle(nullptr);
        WGPUSurfaceSourceWindowsHWND surface_src_hwnd = {};
        surface_src_hwnd.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
        surface_src_hwnd.hinstance = hinstance;
        surface_src_hwnd.hwnd = hwnd;
        surface_descriptor.nextInChain = &surface_src_hwnd.chain;
        surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    }
#else
#error "Unsupported WebGPU native platform!"
#endif
    return surface;
}

#endif

static Result<std::shared_ptr<Mesh>> create_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    std::array<uint16_t, 36> indices{
        0, 1, 2,
        2, 3, 0, // front

        20, 21, 22,
        22, 23, 20, // back

        4, 5, 6,
        6, 7, 4, // right

        12, 13, 14,
        14, 15, 12, // left

        8, 9, 10,
        10, 11, 8, // top

        16, 17, 18,
        18, 19, 16, // bottom
    };
    // clang-format on

    std::array<glm::vec3, 24> vertices{
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // front
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // back
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // left
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // right
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z), // top
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // bottom
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
    };

    std::array<glm::vec2, 24> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    std::array<glm::vec3, 24> normals{
        glm::vec3(0.0, 0.0, 1.0), // front
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),

        glm::vec3(0.0, 0.0, -1.0), // back
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),

        glm::vec3(1.0, 0.0, 0.0), // left
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),

        glm::vec3(-1.0, 0.0, 0.0), // right
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),

        glm::vec3(0.0, 1.0, 0.0), // top
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),

        glm::vec3(0.0, -1.0, 0.0), // bottom
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };

    return Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, normals, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint16);
}

static Result<std::shared_ptr<Mesh>> create_wireframe_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    std::array<uint16_t, 48> indices{
        0, 1, 1, 2, 2, 3, 3, 0, // front
        4, 5, 5, 6, 6, 7, 7, 4, // back
        8, 9, 9, 10, 10, 11, 11, 8, // left
        12, 13, 13, 14, 14, 15, 15, 12, // right
        16, 17, 17, 18, 18, 19, 19, 16, // top
        20, 21, 21, 22, 22, 23, 23, 20, // bottom
    };
    // clang-format on

    std::array<glm::vec3, 24> vertices{
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // front
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // back
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // left
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // right
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z), // top
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // bottom
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
    };

    return Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, std::span<glm::vec3>(), std::span<std::byte>(), WGPUIndexFormat_Uint16);
}

#define SHADOWMAP_RESOLUTION 2048

Result<void> Renderer::init(const Window& window, InitFlags flags)
{
    singleton = this;

#ifndef __platform_web
    WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;

    (void)flags;

    const WGPUInstanceFeatureName features[]{
        WGPUInstanceFeatureName_TimedWaitAny,
    };
    instance_desc.requiredFeatureCount = sizeof(features) / sizeof(WGPUInstanceFeatureName);
    instance_desc.requiredFeatures = features;

    m_instance = wgpuCreateInstance(&instance_desc);
#else
    m_instance = wgpuCreateInstance(nullptr);
#endif

    m_surface = create_surface(m_instance, window.get_window_ptr());
    ERR_COND_R(m_surface == nullptr, "Unable to create the surface", Error(ErrorKind::BadDriver));

#ifdef __platform_web
    // On the web we use glue code to acquire a WGPUDevice.
    m_device = emscripten_webgpu_get_device();
    if (!m_device)
        return Error(ErrorKind::BadDriver);
#else
    m_adapter = request_adapter_sync(m_instance);
    ERR_COND_R(m_adapter == nullptr, "Unable to acquire the adapter", Error(ErrorKind::BadDriver));

    const WGPUFeatureName required_features[] = {
        // (WGPUFeatureName)WGPUNativeFeature_PipelineStatisticsQuery,
        WGPUFeatureName_Depth32FloatStencil8,
    };

    WGPUDeviceDescriptor device_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    device_desc.requiredFeatures = required_features;
    device_desc.requiredFeatureCount = sizeof(required_features) / sizeof(WGPUFeatureName);
    device_desc.defaultQueue = WGPUQueueDescriptor{.nextInChain = nullptr, .label = WGPU_STRING_VIEW_INIT};
    device_desc.deviceLostCallbackInfo = WGPUDeviceLostCallbackInfo{
        .nextInChain = nullptr,
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback = [](const WGPUDevice *, WGPUDeviceLostReason, WGPUStringView, void *, void *) {},
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };
    device_desc.uncapturedErrorCallbackInfo = WGPUUncapturedErrorCallbackInfo{
        .nextInChain = nullptr,
        .callback = [](const WGPUDevice *, WGPUErrorType, WGPUStringView message, void *, void *)
        {
            println("{}", std::string_view(message.data, message.length));

            Stacktrace::record();
            Stacktrace::current().print();
        },
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };

    WGPULimits limits{};
    wgpuAdapterGetLimits(m_adapter, &limits);

    device_desc.requiredLimits = &limits;

    m_device = request_device_sync(m_instance, m_adapter, device_desc);
    ERR_COND_R(m_device == nullptr, "Unable to create the device", Error(ErrorKind::NoSuitableDevice));
#endif

    m_queue = wgpuDeviceGetQueue(m_device);
    ERR_COND_R(m_queue == nullptr, "Unable to retrieve the queue", Error(ErrorKind::NoSuitableDevice));

    m_env_2d_buffer = TRY(Buffer::create(sizeof(glm::mat4), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_sky_buffer = TRY(Buffer::create(sizeof(SkyUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    m_fw_camera = TRY(Buffer::create(sizeof(FwCamera), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_fw_camera_rel = TRY(Buffer::create(sizeof(FwCamera), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_fw_world_env = TRY(Buffer::create(sizeof(FwWorldEnv), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_fw_shadowmap_camera = TRY(Buffer::create(sizeof(FwCamera), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_fw_pp_buffer = TRY(Buffer::create(sizeof(PostProcessUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_portal_buffer = TRY(Buffer::create(sizeof(FwModel), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    m_fw_shadowmap = TRY(Texture::create(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));

    m_color_rect_shader = TRY(Shader::load_from_path("assets/shaders/ui/color_rect.wgsl"));
    m_color_rect_shader->set_binding("env", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_color_rect_shader->set_binding("uniforms", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_color_rect_shader->create_bind_group_layout();

    m_texture_rect_shader = TRY(Shader::load_from_path("assets/shaders/ui/texture_rect.wgsl"));
    m_texture_rect_shader->set_binding("env", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_texture_rect_shader->set_binding("uniforms", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read));
    m_texture_rect_shader->set_binding("image", Binding::Texture(WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_texture_rect_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_texture_rect_shader->create_bind_group_layout();

    m_fw_text_shader = TRY(Shader::load_from_path("assets/shaders/ui/text.wgsl"));
    m_fw_text_shader->set_binding("env", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_fw_text_shader->set_binding("uniforms", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_text_shader->set_binding("bitmap", Binding::Texture(WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_fw_text_shader->create_bind_group_layout();

    m_preview_block_shader = TRY(Shader::load_from_path("assets/shaders/block_preview.wgsl"));
    m_preview_block_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_preview_block_shader->set_binding("images", Binding::Texture(WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read, WGPUTextureViewDimension_2DArray));
    m_preview_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_preview_block_shader->create_bind_group_layout();

    m_missing_texture = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding));
    m_missing_texture->update(std::span((std::byte *)missing_texture_data, 16 * 16 * sizeof(uint32_t)));

    // Create resources for SSAO
    std::uniform_real_distribution<float> random_floats(0.0, 1.0);
    std::default_random_engine generator;

    std::array<glm::vec4, 64> ssao_kernel{};
    for (size_t i = 0; i < 64; i++)
    {
        glm::vec3 sample(random_floats(generator) * 2.0 - 1.0, random_floats(generator) * 2.0 - 1.0, random_floats(generator));
        sample = glm::normalize(sample);
        sample *= random_floats(generator);

        float scale = float(i) / 64.0f;
        scale = math::lerp(0.1f, 1.0f, scale * scale);

        ssao_kernel[i] = glm::vec4(sample * scale, 0.0);
    }

    m_ssao_uniform_buffer = TRY(Buffer::create(sizeof(SSAOUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_ssao_uniform_buffer->update_struct(SSAOUniforms(ssao_kernel));

    std::array<glm::vec4, 16> ssao_noise{};
    for (size_t i = 0; i < 16; i++)
    {
        const glm::vec4 noise(random_floats(generator) * 2.0 - 1.0, random_floats(generator) * 2.0 - 1.0, 0, 0);
        ssao_noise[i] = noise;
    }

    m_ssao_noise_texture = TRY(Texture::create(4, 4, WGPUTextureFormat_RGBA32Float, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst));
    m_ssao_noise_texture->update(std::as_bytes(std::span(ssao_noise)));

    m_fw_model_shader = TRY(Shader::load_from_path("assets/shaders/fw/model.wgsl"));
    m_fw_model_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_fw_model_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_model_shader->set_binding("global_model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_fw_model_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 3, BindingAccess::Read));
    m_fw_model_shader->set_binding("uvs", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 4, BindingAccess::Read));
    m_fw_model_shader->set_binding("texture", Binding::Texture(WGPUShaderStage_Fragment, 0, 5, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_fw_model_shader->set_sampler("texture", SamplerDescriptor(WGPUFilterMode_Nearest, WGPUFilterMode_Nearest));
    m_fw_model_shader->set_binding("shadowmap", Binding::Texture(WGPUShaderStage_Fragment, 0, 7, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUTextureSampleType_Depth, WGPUSamplerBindingType_Comparison));
    m_fw_model_shader->set_sampler("shadowmap", SamplerDescriptor{.compare = WGPUCompareFunction_LessEqual, .address_mode = {.u = WGPUAddressMode_ClampToEdge, .v = WGPUAddressMode_ClampToEdge}});
    m_fw_model_shader->create_bind_group_layout();

    m_fw_item_block_shader = TRY(Shader::load_from_path("assets/shaders/fw/itemblock.wgsl"));
    m_fw_item_block_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_fw_item_block_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_item_block_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_fw_item_block_shader->set_binding("images", Binding::Texture(WGPUShaderStage_Fragment, 0, 3, BindingAccess::Read, WGPUTextureViewDimension_2DArray));
    m_fw_item_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_fw_item_block_shader->set_binding("shadowmap", Binding::Texture(WGPUShaderStage_Fragment, 0, 5, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUTextureSampleType_Depth, WGPUSamplerBindingType_Comparison));
    m_fw_item_block_shader->set_sampler("shadowmap", SamplerDescriptor{.compare = WGPUCompareFunction_LessEqual, .address_mode = {.u = WGPUAddressMode_ClampToEdge, .v = WGPUAddressMode_ClampToEdge}});
    m_fw_item_block_shader->create_bind_group_layout();

    m_fw_item_shader = TRY(Shader::load_from_path("assets/shaders/fw/item.wgsl"));
    m_fw_item_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_fw_item_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_item_shader->set_binding("image", Binding::Texture(WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_fw_item_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_fw_item_shader->create_bind_group_layout();

    m_fw_chunk_shader = TRY(Shader::load_from_path("assets/shaders/fw/chunk.wgsl"));
    m_fw_chunk_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_chunk_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_fw_chunk_shader->set_binding("images", Binding::Texture(WGPUShaderStage_Fragment, 0, 3, BindingAccess::Read, WGPUTextureViewDimension_2DArray));
    m_fw_chunk_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_fw_chunk_shader->set_binding("shadowmap", Binding::Texture(WGPUShaderStage_Fragment, 0, 5, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUTextureSampleType_Depth, WGPUSamplerBindingType_Comparison));
    m_fw_chunk_shader->set_sampler("shadowmap", SamplerDescriptor{.compare = WGPUCompareFunction_LessEqual, .address_mode = {.u = WGPUAddressMode_ClampToEdge, .v = WGPUAddressMode_ClampToEdge}});
    m_fw_chunk_shader->create_bind_group_layout();

    m_fw_water_shader = TRY(Shader::load_from_path("assets/shaders/fw/water.wgsl"));
    m_fw_water_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_water_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_fw_water_shader->set_binding("image", Binding::Texture(WGPUShaderStage_Fragment, 0, 3, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_fw_water_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_fw_water_shader->set_binding("shadowmap", Binding::Texture(WGPUShaderStage_Fragment, 0, 5, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUTextureSampleType_Depth, WGPUSamplerBindingType_Comparison));
    m_fw_water_shader->set_sampler("shadowmap", SamplerDescriptor{.compare = WGPUCompareFunction_LessEqual, .address_mode = {.u = WGPUAddressMode_ClampToEdge, .v = WGPUAddressMode_ClampToEdge}});
    m_fw_water_shader->create_bind_group_layout();

    m_fw_chunk_shadowmap_shader = TRY(Shader::load_from_path("assets/shaders/fw/chunk_shadowmap.wgsl"));
    m_fw_chunk_shadowmap_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_chunk_shadowmap_shader->create_bind_group_layout();

    m_fw_colored_shader = TRY(Shader::load_from_path("assets/shaders/fw/colored.wgsl"));
    m_fw_colored_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_fw_colored_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_fw_colored_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_fw_colored_shader->create_bind_group_layout();

    m_sky_shader = TRY(Shader::load_from_path("assets/shaders/sky.wgsl"));
    m_sky_shader->set_binding("uniforms", Binding::UniformBuffer(WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read));
    m_sky_shader->create_bind_group_layout();

    m_fw_pp_shader = TRY(Shader::load_from_path("assets/shaders/fw/postprocess.wgsl"));
    m_fw_pp_shader->set_binding("uniforms", Binding::UniformBuffer(WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read));
    m_fw_pp_shader->set_binding("ssao", Binding::UniformBuffer(WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read));
    m_fw_pp_shader->set_binding("albedo", Binding::Texture(WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_fw_pp_shader->set_binding("depth", Binding::Texture(WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUTextureSampleType_Depth, WGPUSamplerBindingType_Filtering));
    m_fw_pp_shader->create_bind_group_layout();

    m_portal_shader = TRY(Shader::load_from_path("assets/shaders/portal.wgsl"));
    m_portal_shader->set_binding("model", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_portal_shader->set_binding("camera", Binding::UniformBuffer(WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_portal_shader->set_binding("world_env", Binding::UniformBuffer(WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_portal_shader->create_bind_group_layout();

    m_fw_texture_rect_mat = Material::create(m_texture_rect_shader, MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal, WGPUCullMode_None, WGPUVertexFormat_Float32x2);
    m_fw_model_mat = Material::create(m_fw_model_shader, MaterialFlagBits::None, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);
    m_fw_color_rect_mat = Material::create(m_color_rect_shader, MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_None, WGPUVertexFormat_Float32x2);
    m_fw_item_block_mat = Material::create(m_fw_item_block_shader, MaterialFlagBits::None, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);
    m_fw_shadowmap_cam_mat = Material::create(m_fw_colored_shader, MaterialFlagBits::None, WGPUCullMode_None, WGPUVertexFormat_Float32x2);
    m_fw_item_block_mat = Material::create(m_fw_item_block_shader, MaterialFlagBits::None, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);
    m_fw_item_mat = Material::create(m_fw_item_shader, MaterialFlagBits::Transparency, WGPUCullMode_None, WGPUVertexFormat_Float32x2);
    m_portal_mat = Material::create(m_portal_shader, MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV | MaterialFlagBits::StencilMask, WGPUCullMode_None, WGPUVertexFormat_Float32x2);

    std::vector<InstanceAttribute> chunk_attribs{InstanceAttribute(0, WGPUVertexFormat_Float32x3)};
    m_fw_chunk_mat = Material::create(m_fw_chunk_shader, MaterialFlagBits::Stencil, WGPUCullMode_Back, WGPUVertexFormat_Float32x4, Instance(chunk_attribs, sizeof(glm::vec3)));
    m_fw_chunk_shadowmap_mat = Material::create(m_fw_chunk_shadowmap_shader, MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_Back, WGPUVertexFormat_Float32x4, Instance(chunk_attribs, sizeof(glm::vec3)));
    m_fw_water_mat = Material::create(m_fw_water_shader, MaterialFlagBits::Transparency, WGPUCullMode_Back, WGPUVertexFormat_Float32x2, Instance(chunk_attribs, sizeof(glm::vec3)));

    m_fw_colored_mat = Material::create(m_fw_colored_shader, MaterialFlagBits::NoUV, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);
    m_fw_colored_shadowmap_mat = Material::create(m_fw_colored_shader, MaterialFlagBits::NoUV, WGPUCullMode_Front, WGPUVertexFormat_Float32x2);

    m_sky_mat = Material::create(m_sky_shader, MaterialFlagBits::DisableDepthTest | MaterialFlagBits::NoData | MaterialFlagBits::Stencil, WGPUCullMode_None, WGPUVertexFormat_Float32x2);

    m_fw_pp_mat = Material::create(m_fw_pp_shader, MaterialFlagBits::NoData, WGPUCullMode_None, WGPUVertexFormat_Float32x2);

    std::vector<InstanceAttribute> attribs{InstanceAttribute(offsetof(Font::Instance, bounds), WGPUVertexFormat_Float32x4),
                                           InstanceAttribute(offsetof(Font::Instance, char_pos), WGPUVertexFormat_Float32x2),
                                           InstanceAttribute(offsetof(Font::Instance, scale), WGPUVertexFormat_Float32x2)};
    m_fw_text_mat = Material::create(m_fw_text_shader, MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_None, WGPUVertexFormat_Float32x2, Instance(attribs, sizeof(Font::Instance)));

    m_wireframe_dbg_mat = Material::create(m_fw_colored_shader, MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_None, WGPUVertexFormat_Float32x2, Instance(), WGPUPrimitiveTopology_LineList);

    m_cube_mesh = TRY(create_cube_mesh());
    m_wireframe_chunk_slice_mesh = TRY(create_wireframe_cube_mesh(glm::vec3(16.0)));

    Engine::get().registry().register_all();       // TODO: I dont really like having this here but it needs to be before calling `get_texture_array` and after initializing WebGPU.
    TRY(Engine::get().registry().post_register()); //       Maybe split this function in two (initializing/creating resources).

    m_fw_colored_buffer = TRY(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    FwColored colored(
        glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 78, 0)),
        Color(1.0, 1.0, 1.0, 1.0));
    m_fw_colored_buffer->update_struct(colored);

    m_fw_shadowmap_cam_buffer = TRY(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    m_fw_colored_bg = BindGroup::create(m_fw_colored_shader);
    m_fw_colored_bg->set_param("model", m_fw_colored_buffer);
    m_fw_colored_bg->set_param("camera", m_fw_camera);
    m_fw_colored_bg->set_param("world_env", m_fw_world_env);

    m_fw_colored_shadowmap_bg = BindGroup::create(m_fw_colored_shader);
    m_fw_colored_shadowmap_bg->set_param("model", m_fw_colored_buffer);
    m_fw_colored_shadowmap_bg->set_param("camera", m_fw_shadowmap_camera);
    m_fw_colored_shadowmap_bg->set_param("world_env", m_fw_world_env);

    m_fw_shadowmap_cam_bg = BindGroup::create(m_fw_colored_shader);
    m_fw_shadowmap_cam_bg->set_param("model", m_fw_shadowmap_cam_buffer);
    m_fw_shadowmap_cam_bg->set_param("camera", m_fw_camera);
    m_fw_shadowmap_cam_bg->set_param("world_env", m_fw_world_env);

    m_sky_bg = BindGroup::create(m_sky_shader);
    m_sky_bg->set_param("uniforms", m_sky_buffer);

    std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};
    std::array<glm::vec3, 4> vertices{
        glm::vec3(-0.5, -0.5, 0.1),
        glm::vec3(+0.5, -0.5, 0.1),
        glm::vec3(+0.5, +0.5, 0.1),
        glm::vec3(-0.5, +0.5, 0.1),
    };
    std::array<glm::vec2, 4> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };
    m_square_mesh = TRY(Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, {}, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint16));

    std::array<uint16_t, 6> quad_indices{0, 1, 2, 0, 2, 3};
    std::array<glm::vec3, 4> quad_vertices{
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(+0.5, -0.5, 0.0),
        glm::vec3(+0.5, +0.5, 0.0),
        glm::vec3(-0.5, +0.5, 0.0),
    };
    std::array<glm::vec3, 4> quad_normals{
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
    };
    std::array<glm::vec2, 4> quad_uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };
    m_quad_mesh = TRY(Mesh::create_from_data(std::as_bytes(std::span(quad_indices)), quad_vertices, quad_normals, std::as_bytes(std::span(quad_uvs)), WGPUIndexFormat_Uint16));

    m_fw_pp_bg = BindGroup::create(m_fw_pp_shader);
    m_fw_pp_bg->set_param("uniforms", m_fw_pp_buffer);
    m_fw_pp_bg->set_param("ssao", m_ssao_uniform_buffer);

    m_portal_bg = BindGroup::create(m_portal_shader);
    m_portal_bg->set_param("world_env", m_fw_world_env);
    m_portal_bg->set_param("camera", m_fw_camera);
    m_portal_bg->set_param("model", m_portal_buffer);

    m_fw_water_texture = Engine::get().registry().create_texture("assets/textures/water.png");

    Extent2D window_size = window.size();
    configure_surface(window_size.width, window_size.height, VSync::On);

    // WGPUQuerySetDescriptor desc{};
    // desc.type = WGPUQueryType_Occlusion;
    // m_occlusion_set = wgpuDeviceCreateQuerySet(m_device, &desc);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForOther(window.get_window_ptr());

    ImGui_ImplWGPU_InitInfo init_info{};
    init_info.Device = m_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = m_surface_format;
    init_info.DepthStencilFormat = WGPUTextureFormat_Depth32FloatStencil8;
    ImGui_ImplWGPU_Init(&init_info);

    return Result<void>();
}

void Renderer::configure_surface(size_t width, size_t height, VSync vsync)
{
    (void)vsync;

    Extent2D surface_extent(width, height);

#ifndef __platform_web
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);

    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = m_device;
    config.format = WGPUTextureFormat_BGRA8Unorm; // capabilities.formats[0];
    config.usage = capabilities.usages;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = WGPUPresentMode_Immediate; // vsync == VSync::On ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    config.alphaMode = capabilities.alphaModes[0];

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
#else
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = m_device;
    config.format = WGPUTextureFormat_RGBA8Unorm;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = WGPUPresentMode(1); // only FIFO is supported
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
#endif

    wgpuSurfaceConfigure(m_surface, &config);
    m_surface_extent = surface_extent;
    m_surface_format = config.format;

    m_fw_depth_texture = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_Depth32FloatStencil8, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc));
    m_fw_color_texture = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_BGRA8Unorm, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));

    float ratio = float(width) / float(height);
    glm::mat4 ortho_matrix = glm::ortho(0.0f, 1.0f * ratio, 1.0f, 0.0f, -1.0f, 1.0f);
    m_env_2d_buffer->update_struct(ortho_matrix);

    m_fw_pp_bg->set_param("albedo", EXPECT(m_fw_color_texture->get_view()));
    m_fw_pp_bg->set_param("depth", EXPECT(m_fw_depth_texture->get_view(WGPUTextureViewDimension_2D, WGPUTextureAspect_DepthOnly)));
}

void Renderer::draw_legacy(std::function<void()> f)
{
    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", (uint32_t)surface_texture.status);
    // TODO: if status is WGPUSurfaceGetCurrentTextureStatus_Outdated or WGPUSurfaceGetCurrentTextureStatus_Timeout

    WGPUTextureView surface_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
    ERR_COND_R(surface_view == nullptr, "Cannot acquire a swapchain image view");

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    WGPURenderPassDescriptor rp{};
    rp.label = WGPU_STRING_VIEW("imgui Pass");

    WGPURenderPassColorAttachment color_attach{};
    color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attach.loadOp = WGPULoadOp_Clear;
    color_attach.storeOp = WGPUStoreOp_Store;
    color_attach.view = surface_view;
    rp.colorAttachmentCount = 1;
    rp.colorAttachments = &color_attach;

    WGPURenderPassDepthStencilAttachment depth_attach{};
    depth_attach.depthClearValue = 1.0;
    depth_attach.depthLoadOp = WGPULoadOp_Clear;
    depth_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilLoadOp = WGPULoadOp_Clear;
    depth_attach.stencilStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilClearValue = 1;
    depth_attach.view = EXPECT(m_fw_depth_texture->get_view(WGPUTextureViewDimension_2D));
    rp.depthStencilAttachment = &depth_attach;

    WGPURenderPassEncoder render_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &rp);
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    f();

    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), render_encoder);
    wgpuRenderPassEncoderEnd(render_encoder);
    wgpuRenderPassEncoderRelease(render_encoder);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);

    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueSubmit(m_queue, 1, &command_buffer);

#ifdef __platform_web
    emscripten_request_animation_frame([](double, void *)
                                       { return true; }, nullptr);
#else
    wgpuSurfacePresent(m_surface);
#endif

    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(encoder);

    wgpuTextureViewRelease(surface_view);
    wgpuTextureRelease(surface_texture.texture);
}

Result<Cloud> Renderer::create_cloud()
{
    Cloud cloud;
    cloud.uniform.color = Color(0.92, 0.92, 0.92, 1.0);
    cloud.buffer = TRY(Buffer::create(sizeof(FwColored), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
    cloud.bg = BindGroup::create(m_fw_colored_shader);
    cloud.bg->set_param("model", cloud.buffer);
    cloud.bg->set_param("camera", m_fw_camera);
    cloud.bg->set_param("world_env", m_fw_world_env);
    return cloud;
}

bool Renderer::has_cloud(int64_t x, int64_t z)
{
    return m_clouds_set.contains(ChunkPos(x, z));
}

void Renderer::update_clouds(std::shared_ptr<Camera> camera)
{
    glm::vec3 camera_position = camera->get_global_transform().position();
    float time = Engine::get().time() * 1.0f;

    for (size_t i = 0; i < m_clouds.size(); i++)
    {
        float distance = glm::distance2(glm::vec2(camera_position.x, camera_position.z), glm::vec2(float(m_clouds[i].grid_x) * 32.0f, float(m_clouds[i].grid_z) * 32.0f) + glm::vec2(time, 0));
        if (distance > 16.0f * 32.0f)
        {
            const Cloud& cloud = m_clouds[i];
            m_clouds.erase(m_clouds.begin() + (ssize_t)i);
            m_clouds_set.erase(ChunkPos(cloud.grid_x, cloud.grid_z));
            if (i > 0)
                i -= 1;
            i--;
        }
    }

    int64_t cx = int64_t((camera_position.x + time) / 32.0f);
    int64_t cz = int64_t(camera_position.z / 32.0f);

    for (int64_t x = cx - 8; x < cx + 8; x++)
        for (int64_t z = cz - 8; z < cz + 8; z++)
        {
            if (has_cloud(x, z))
                continue;

            float density = m_clouds_noise.sample(glm::vec2(x, z)) / 2.0f + 0.5f;
            if (density > 0.7f)
            {
                Cloud cloud = EXPECT(create_cloud());
                cloud.uniform.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, 0.0f, z) * 32.0f + glm::vec3(0, 270.0f, 0) + glm::vec3(time, 0, 0)) *
                                      glm::scale(glm::identity<glm::mat4>(), glm::vec3(32.0f, 4.0f, 32.0f));

                cloud.buffer->update_struct(cloud.uniform);
                cloud.grid_x = x;
                cloud.grid_z = z;
                m_clouds.push_back(cloud);
                m_clouds_set.insert(ChunkPos(x, z));
            }
        }
}

struct LightMatrices
{
    glm::mat4 view;
    glm::mat4 projection;
};

LightMatrices getStableLightMatrices(const glm::vec3& lightDir,
                                     const glm::vec3& mainCameraTarget,
                                     float frustumSize,
                                     float shadowMapResolution)
{
    // Ensure the light direction vector is normalized
    glm::vec3 normalizedLightDir = glm::normalize(lightDir);

    // 1. Establish a temporary, un-snapped light view matrix.
    // We use a temporary position along the light ray relative to our camera target.
    glm::vec3 tempLightPos = mainCameraTarget + (normalizedLightDir * (frustumSize * 0.5f));

    // Choose a stable up vector. If the light points straight down/up, shift the up vector.
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(normalizedLightDir, upVector)) > 0.99f)
    {
        upVector = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 rawLightView = glm::lookAt(tempLightPos, mainCameraTarget, upVector);

    // 2. Project the world-space camera target into this temporary light space
    glm::vec4 targetInLightSpace = rawLightView * glm::vec4(mainCameraTarget, 1.0f);

    // 3. Calculate how many world units are represented by a single shadow map texel
    float texelSize = frustumSize / shadowMapResolution;

    // 4. Snap the X and Y coordinates to whole texel units to eliminate sub-texel shifting
    targetInLightSpace.x = std::floor(targetInLightSpace.x / texelSize) * texelSize;
    targetInLightSpace.y = std::floor(targetInLightSpace.y / texelSize) * texelSize;

    // 5. Invert the raw view matrix to transform our snapped light-space coordinate back to world space
    glm::mat4 invRawLightView = glm::inverse(rawLightView);
    glm::vec4 stableTargetWorld = invRawLightView * targetInLightSpace;

    // 6. Regenerate the final locked View and Orthographic Projection matrices
    LightMatrices output{};

    // Recalculate stable light position using the snapped world target
    glm::vec3 stableLightPos = glm::vec3(stableTargetWorld) + (normalizedLightDir * (frustumSize * 0.5f));
    output.view = glm::lookAt(stableLightPos, glm::vec3(stableTargetWorld), upVector);

    // Generate standard WebGPU-aligned orthographic bounds [0.0, 1.0] depth distribution
    float halfSize = frustumSize * 0.5f;

    // Note: GLM defaults to Vulkan/WebGPU depth conventions [0.0, 1.0] when GLM_FORCE_DEPTH_ZERO_TO_ONE is defined.
    // If you haven't defined that macro globally, use glm::orthoLH_ZO or glm::orthoRH_ZO based on your coordinate system.
    output.projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, frustumSize);

    return output;
}

void Renderer::draw_forward(const std::shared_ptr<World>& world)
{
    ZoneScoped;

    WGPUSurfaceTexture surface_texture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", (uint32_t)surface_texture.status);
    // TODO: if status is WGPUSurfaceGetCurrentTextureStatus_Outdated or WGPUSurfaceGetCurrentTextureStatus_Timeout

    WGPUTextureView surface_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
    ERR_COND_R(surface_view == nullptr, "Cannot acquire a swapchain image view");
    std::shared_ptr<Texture> surface_tex = Texture::create_from_handle(surface_texture.texture);

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    const int current_dim = world->get_player()->get_dimension();
    const int portal_dim = (current_dim + 1) % 2;

    // println("current = {}, portal = {}", current_dim, portal_dim);

    draw_dimension_forward(encoder, world, current_dim, false);
    draw_dimension_forward(encoder, world, portal_dim, true);

    WGPURenderPassColorAttachment output_color_attach = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    output_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    output_color_attach.loadOp = WGPULoadOp_Clear;
    output_color_attach.storeOp = WGPUStoreOp_Store;
    output_color_attach.view = surface_view;

    WGPURenderPassDescriptor postprocess_pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    postprocess_pass_desc.colorAttachments = &output_color_attach;
    postprocess_pass_desc.colorAttachmentCount = 1;

    WGPURenderPassEncoder postprocess_pass = wgpuCommandEncoderBeginRenderPass(encoder, &postprocess_pass_desc);
    const RenderPass postprocess_pass_info(postprocess_pass, std::nullopt, {m_surface_format});
    draw_fullscreen(postprocess_pass_info, m_fw_pp_mat, m_fw_pp_bg, 0);
    wgpuRenderPassEncoderEnd(postprocess_pass);
    wgpuRenderPassEncoderRelease(postprocess_pass);

    // Finally one last pass for rendering the UI.
    WGPURenderPassColorAttachment color_load_attach = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color_load_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    color_load_attach.loadOp = WGPULoadOp_Load;
    color_load_attach.storeOp = WGPUStoreOp_Store;
    color_load_attach.view = surface_view;

    WGPURenderPassDescriptor ui_pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    ui_pass_desc.label = WGPU_STRING_VIEW("Color pass");
    ui_pass_desc.colorAttachmentCount = 1;
    ui_pass_desc.colorAttachments = &color_load_attach;

    {
        ZoneScopedN("draw ui");

        WGPURenderPassEncoder ui_pass = wgpuCommandEncoderBeginRenderPass(encoder, &ui_pass_desc);
        const RenderPass ui_pass_info(ui_pass, std::nullopt, {m_surface_format});
        for (const std::shared_ptr<Entity>& entity : world->get_dimension(world->get_player()->get_dimension()).get_entities())
            entity->draw_ui(ui_pass_info);
        wgpuRenderPassEncoderEnd(ui_pass);
        wgpuRenderPassEncoderRelease(ui_pass);
    }

    // Submit everything to the GPU.
    WGPUCommandBuffer command_buffer;
    {
        ZoneScopedN("finish encoder");

        command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuCommandEncoderRelease(encoder);
    }

    {
        ZoneScopedN("submit");
        std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
        wgpuQueueSubmit(m_queue, 1, &command_buffer);
    }

#ifdef __platform_web
    emscripten_request_animation_frame([](double, void *)
                                       { return true; }, nullptr);
#else
    wgpuSurfacePresent(m_surface);
#endif

    wgpuCommandBufferRelease(command_buffer);
    wgpuTextureViewRelease(surface_view);
    wgpuTextureRelease(surface_texture.texture);
}

void Renderer::draw_dimension_forward(WGPUCommandEncoder encoder, const std::shared_ptr<World>& world, int dimension, bool inside_portal)
{
    // bool foreign = world->get_player()->get_dimension() != dimension;
    std::shared_ptr<Camera> active_camera = world->get_player()->get_camera();

    FwCamera camera{};
    camera.view_projection = active_camera->get_view_proj_matrix();
    m_fw_camera->update_struct(camera);

    camera.view_projection = active_camera->get_projection_matrix();
    m_fw_camera_rel->update_struct(camera);

    const float shadowmap_range = float(world->get_render_distance()) * 34.0f;
    const glm::vec3 light_target = active_camera->get_global_transform().position();
    const glm::vec3 light_dir = glm::normalize(glm::vec3(1, 1, 0));
    // const float light_distance = 100.0;

    // const glm::mat4 shadowmap_proj = glm::ortho(-shadowmap_range, shadowmap_range, -shadowmap_range, shadowmap_range, -1.0f, 300.0f);
    // const glm::mat4 shadowmap_view = glm::lookAt(light_target + light_dir * light_distance, light_target, glm::vec3(0, 1, 0));

    LightMatrices light = getStableLightMatrices(light_dir, light_target, shadowmap_range, SHADOWMAP_RESOLUTION);

    update_clouds(active_camera);

    world->get_dimension(dimension).update_sun(light.projection * light.view);

    FwColored shadowmap_cam(
        glm::inverse(light.view) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(100.0) * 2.0f),
        Colors::white);
    m_fw_shadowmap_cam_buffer->update_struct(shadowmap_cam);

    FwCamera shadowmap_camera{};
    shadowmap_camera.view_projection = light.projection * light.view;
    m_fw_shadowmap_camera->update_struct(shadowmap_camera);

    FwWorldEnv world_env{};
    world_env.light_view_projection = shadowmap_camera.view_projection;
    world_env.light_dir = light_dir;

    m_fw_world_env->update_struct(world_env);

    m_fw_pp.camera_proj = active_camera->get_projection_matrix();
    m_fw_pp.inverse_camera_proj = glm::inverse(m_fw_pp.camera_proj);
    m_fw_pp.near = active_camera->near_plane();
    m_fw_pp.far = active_camera->far_plane();
    m_fw_pp_buffer->update_struct(m_fw_pp);

    FwModel model{};
    model.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0, 100, 0)) *
                  glm::scale(glm::identity<glm::mat4>(), glm::vec3(100, 100, 100));
    m_portal_buffer->update_struct(model);

    const uint32_t stencil_mask = inside_portal ? 2 : 1;

    // Generate a shadowmap by doing a depth-only pass from the point of view of the "sun".
    WGPURenderPassDepthStencilAttachment shadowmap_attach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    shadowmap_attach.depthClearValue = 1.0;
    shadowmap_attach.depthLoadOp = WGPULoadOp_Clear;
    shadowmap_attach.depthStoreOp = WGPUStoreOp_Store;
    shadowmap_attach.stencilLoadOp = WGPULoadOp_Load;
    shadowmap_attach.stencilStoreOp = WGPUStoreOp_Store;
    shadowmap_attach.stencilClearValue = 1;
    shadowmap_attach.view = EXPECT(m_fw_shadowmap->get_view(WGPUTextureViewDimension_2D));

    WGPURenderPassDescriptor shadowmap_pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    shadowmap_pass_desc.label = WGPU_STRING_VIEW("Shadowmap");
    shadowmap_pass_desc.depthStencilAttachment = &shadowmap_attach;

    WGPURenderPassEncoder shadowmap_pass = wgpuCommandEncoderBeginRenderPass(encoder, &shadowmap_pass_desc);
    // draw_world(world, RenderPass(shadowmap_pass, RenderTarget(m_fw_shadowmap->format()), {}), WorldFlagBits::Shadowmap, world->get_dimension(0).get_sun_visible_chunks());
    wgpuRenderPassEncoderEnd(shadowmap_pass);
    wgpuRenderPassEncoderRelease(shadowmap_pass);

    // A depth-only pass will optimize the rendering by only processing fragment colors once per pixels. Only opaque objects are preprocessed.
    WGPURenderPassDepthStencilAttachment depth_attach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    depth_attach.depthClearValue = 1.0;
    depth_attach.depthLoadOp = inside_portal ? WGPULoadOp_Load : WGPULoadOp_Clear;
    depth_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilLoadOp = inside_portal ? WGPULoadOp_Load : WGPULoadOp_Clear;
    depth_attach.stencilStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilClearValue = 1;
    depth_attach.view = EXPECT(m_fw_depth_texture->get_view(WGPUTextureViewDimension_2D));

    WGPURenderPassDescriptor depth_prepass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    depth_prepass_desc.label = WGPU_STRING_VIEW("Depth Prepass");
    depth_prepass_desc.depthStencilAttachment = &depth_attach;

    WGPURenderPassEncoder depth_pass = wgpuCommandEncoderBeginRenderPass(encoder, &depth_prepass_desc);
    draw_world(world, RenderPass(depth_pass, RenderTarget(m_fw_depth_texture->format()), {}), WorldFlags(), world->get_dimension(dimension).get_visible_chunks(), stencil_mask);
    wgpuRenderPassEncoderEnd(depth_pass);
    wgpuRenderPassEncoderRelease(depth_pass);

    // The color pass shade each pixels, the depth prepass prevent overshading for opaque objects.
    WGPURenderPassDepthStencilAttachment depth_load_attach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    depth_load_attach.depthClearValue = 1.0;
    depth_load_attach.depthLoadOp = inside_portal ? WGPULoadOp_Load : WGPULoadOp_Load;
    depth_load_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_load_attach.stencilLoadOp = inside_portal ? WGPULoadOp_Load : WGPULoadOp_Clear;
    depth_load_attach.stencilStoreOp = WGPUStoreOp_Store;
    depth_load_attach.stencilClearValue = (uint32_t)dimension + 1;
    depth_load_attach.view = EXPECT(m_fw_depth_texture->get_view(WGPUTextureViewDimension_2D));

    WGPURenderPassColorAttachment color_attach = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    color_attach.loadOp = inside_portal ? WGPULoadOp_Load : WGPULoadOp_Clear;
    color_attach.storeOp = WGPUStoreOp_Store;
    color_attach.view = EXPECT(m_fw_color_texture->get_view(WGPUTextureViewDimension_2D));

    WGPURenderPassDescriptor color_pass_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    color_pass_desc.label = WGPU_STRING_VIEW("Color pass");
    color_pass_desc.colorAttachmentCount = 1;
    color_pass_desc.colorAttachments = &color_attach;
    color_pass_desc.depthStencilAttachment = &depth_load_attach;

    WGPURenderPassEncoder color_pass = wgpuCommandEncoderBeginRenderPass(encoder, &color_pass_desc);
    const RenderPass color_pass_info(color_pass, RenderTarget(m_fw_depth_texture->format()), {m_surface_format});
    draw_fullscreen(color_pass_info, m_sky_mat, m_sky_bg, stencil_mask);
    draw_world(world, color_pass_info, WorldFlags(), world->get_dimension(dimension).get_visible_chunks(), stencil_mask);
    draw_world(world, color_pass_info, WorldFlagBits::Water, world->get_dimension(dimension).get_visible_chunks(), stencil_mask);

    if (!inside_portal)
    {
        // TODO: differentiate between current player rendering and other entities.
        for (std::shared_ptr<Entity> entity : world->get_dimension(dimension).get_entities())
            entity->draw(color_pass_info);
    }

    for (size_t i = 0; i < m_clouds.size(); i++)
        draw(color_pass_info, m_cube_mesh, m_fw_colored_mat, m_clouds[i].bg);

    draw(color_pass_info, m_quad_mesh, m_fw_shadowmap_cam_mat, m_fw_shadowmap_cam_bg); // Quad placed at the origin of the "sun"

    // Don't draw the portal inside the portal view otherwise it will create problems.
    // if (!inside_portal)
    //     draw(color_pass_info, m_quad_mesh, m_portal_mat, m_portal_bg, nullptr, 1, 0x2);

    wgpuRenderPassEncoderEnd(color_pass);
    wgpuRenderPassEncoderRelease(color_pass);
}

void Renderer::draw(const RenderPass& pass, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const std::shared_ptr<BindGroup>& bg, const std::shared_ptr<Buffer>& instance_buffer, size_t instance_count, std::optional<uint32_t> stencil)
{
    wgpuRenderPassEncoderSetPipeline(pass.encoder, material->get_pipeline(pass));
    wgpuRenderPassEncoderSetBindGroup(pass.encoder, 0, bg->get_bind_group(), 0, nullptr);
    wgpuRenderPassEncoderSetIndexBuffer(pass.encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());

    if (stencil.has_value())
        wgpuRenderPassEncoderSetStencilReference(pass.encoder, stencil.value());

    size_t buffer_index = 1;
    if (!material->flags().has_any(MaterialFlagBits::NoNormal))
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
    if (!material->flags().has_any(MaterialFlagBits::NoUV))
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

    if (instance_buffer != nullptr)
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, instance_buffer->handle(), 0, instance_buffer->size());

    wgpuRenderPassEncoderDrawIndexed(pass.encoder, mesh->vertex_count(), instance_count, 0, 0, 0);
}

void Renderer::draw_world(const std::shared_ptr<World>& world, const RenderPass& pass, WorldFlags flags, const std::span<const RenderableChunk>& chunks, uint32_t stencil)
{
    ZoneScoped;

    const std::shared_ptr<Camera> camera = world->get_player()->get_camera();
    WGPURenderPassEncoder encoder = pass.encoder;

    if (camera == nullptr)
        return;

    std::shared_ptr<Material> mat = flags.has_any(WorldFlagBits::Shadowmap) ? m_fw_chunk_shadowmap_mat : (flags.has_any(WorldFlagBits::Water) ? m_fw_water_mat : m_fw_chunk_mat);
    wgpuRenderPassEncoderSetPipeline(encoder, mat->get_pipeline(pass));
    wgpuRenderPassEncoderSetStencilReference(encoder, stencil);

    for (const auto& r : chunks)
    {
        const Chunk::Slice& slice = r.chunk->get_slices()[r.slice_index];
        std::shared_ptr<BindGroup> bg = flags.has_any(WorldFlagBits::Shadowmap) ? slice.mesh_shadowmap_bg : (flags.has_any(WorldFlagBits::Water) ? slice.water_bg : slice.mesh_bg);

        if (flags.has_any(WorldFlagBits::Water) && slice.water_mesh == nullptr)
            continue;

        if (!flags.has_any(WorldFlagBits::Water) && slice.mesh == nullptr)
            continue;

        wgpuRenderPassEncoderSetBindGroup(encoder, 0, bg->get_bind_group(), 0, nullptr);

        const std::shared_ptr<Mesh>& mesh = flags.has_any(WorldFlagBits::Water) ? slice.water_mesh : slice.mesh;
        wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());

        size_t buffer_index = 1;
        if (!mat->flags().has_any(MaterialFlagBits::NoNormal))
            wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
        if (!mat->flags().has_any(MaterialFlagBits::NoUV))
            wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

        wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, r.chunk->get_instance_buffer()->handle(), 0, r.chunk->get_instance_buffer()->size());
        wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, r.slice_index);
    }

    if (m_chunk_debug)
    {
        for (const auto& r : chunks)
        {
            std::shared_ptr<BindGroup> dbg_bg = BindGroup::create(m_fw_colored_shader);
            dbg_bg->set_param("camera", m_fw_camera);
            dbg_bg->set_param("world_env", m_fw_world_env);

            FwColored colored{};
            colored.color = Colors::yellow;
            colored.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(float(r.chunk->pos().x) * 16.0f + 8.0f - 0.5f, float(r.slice_index) * 16.0f + 8.0f - 0.5f, float(r.chunk->pos().z) * 16.0f + 8.0f - 0.5f));

            std::shared_ptr<Buffer> dbg_buffer = EXPECT(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
            dbg_buffer->update_struct(colored);
            dbg_bg->set_param("model", dbg_buffer);

            draw(pass, m_wireframe_chunk_slice_mesh, m_wireframe_dbg_mat, dbg_bg);
        }
    }
}

void Renderer::draw_all_world(const std::shared_ptr<World>& world, const RenderPass& pass, WorldFlags flags)
{
    ZoneScoped;

    const Dimension& dim = world->get_dimension(0);
    const std::shared_ptr<Camera> camera = world->get_player()->get_camera();
    WGPURenderPassEncoder encoder = pass.encoder;

    if (camera == nullptr)
        return;

    std::shared_ptr<Material> mat = flags.has_any(WorldFlagBits::Shadowmap) ? m_fw_chunk_shadowmap_mat : (flags.has_any(WorldFlagBits::Water) ? m_fw_water_mat : m_fw_chunk_mat);
    wgpuRenderPassEncoderSetPipeline(encoder, mat->get_pipeline(pass));

    for (const auto& [key, chunk] : dim.get_chunks())
    {
        ChunkPos pos = chunk->pos();
        AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                         .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

        if (!flags.has_any(WorldFlagBits::NoFrustumCheck) && !camera->frustum().contains(aabb))
            continue;

        const Chunk::Slice *slices = chunk->get_slices();

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            Chunk::Slice slice = slices[i];

            if ((flags.has_any(WorldFlagBits::Water) && slice.water_mesh == nullptr) || (!flags.has_any(WorldFlagBits::Water) && slice.mesh == nullptr))
                continue;

            ChunkPos pos = chunk->pos();
            AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
                             .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            if (!flags.has_any(WorldFlagBits::NoFrustumCheck) && !camera->frustum().contains(aabb))
                continue;

            std::shared_ptr<BindGroup> bg = flags.has_any(WorldFlagBits::Shadowmap) ? slice.mesh_shadowmap_bg : (flags.has_any(WorldFlagBits::Water) ? slice.water_bg : slice.mesh_bg);

            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bg->get_bind_group(), 0, nullptr);

            const std::shared_ptr<Mesh>& mesh = flags.has_any(WorldFlagBits::Water) ? slice.water_mesh : slice.mesh;
            wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());

            size_t buffer_index = 1;
            if (!mat->flags().has_any(MaterialFlagBits::NoNormal))
                wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
            if (!mat->flags().has_any(MaterialFlagBits::NoUV))
                wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

            wgpuRenderPassEncoderSetVertexBuffer(encoder, buffer_index++, chunk->get_instance_buffer()->handle(), 0, chunk->get_instance_buffer()->size());
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, i);
        }
    }
}

void Renderer::draw_fullscreen(const RenderPass& pass, std::shared_ptr<Material> material, std::shared_ptr<BindGroup> bg, uint32_t stencil)
{
    wgpuRenderPassEncoderSetPipeline(pass.encoder, material->get_pipeline(pass));
    wgpuRenderPassEncoderSetStencilReference(pass.encoder, stencil);
    wgpuRenderPassEncoderSetBindGroup(pass.encoder, 0, bg->get_bind_group(), 0, nullptr);
    wgpuRenderPassEncoderDraw(pass.encoder, 3, 1, 0, 0);
}

void Renderer::set_fog(glm::vec4 color, float distance)
{
    m_fw_pp.fog_color = color;
    m_fw_pp.fog_distance = distance;

    std::array<PostProcessUniforms, 1> u{m_fw_pp};
    m_fw_pp_buffer->update(std::as_bytes(std::span(u)));
}

void Renderer::set_sky(glm::vec4 color)
{
    std::array<SkyUniforms, 1> u{SkyUniforms(color)};
    m_sky_buffer->update(std::as_bytes(std::span(u)));
}

void Renderer::set_underwater(bool v)
{
    m_fw_pp.underwater = v;

    std::array<PostProcessUniforms, 1> u{m_fw_pp};
    m_fw_pp_buffer->update(std::as_bytes(std::span(u)));
}

std::span<const uint8_t> Renderer::get_missing_texture_data() const
{
    return std::span((uint8_t *)missing_texture_data, 16 * 16 * sizeof(uint32_t));
}
