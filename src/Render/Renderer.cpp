#include "Render/Renderer.hpp"

#include "Core/Assert.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Math.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Profiler.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Variant.hpp"
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>

#include <mutex>
#include <random>

// clang-format off
static const uint32_t missing_texture_data[16 * 16]{
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF, 0xFF00FFFF,
};
// clang-format on

#include "Shaders.cpp"

static WGPUTextureDimension convert_texture_dimension(WGPUTextureViewDimension dimension)
{
    switch (dimension)
    {
    case WGPUTextureViewDimension_1D:
        return WGPUTextureDimension_1D;
    case WGPUTextureViewDimension_2D:
    case WGPUTextureViewDimension_2DArray:
    case WGPUTextureViewDimension_Cube:
    case WGPUTextureViewDimension_CubeArray:
        return WGPUTextureDimension_2D;
    case WGPUTextureViewDimension_3D:
        return WGPUTextureDimension_3D;
    default:
        break;
    }
    return {};
}

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

Buffer::~Buffer()
{
    wgpuBufferRelease(m_buffer);
    Renderer::get().m_device_memory_freed += m_size;
}

Result<Ref<Buffer>> Buffer::create(size_t size, WGPUBufferUsage usage, BufferVisibility visibility)
{
    WGPUBufferDescriptor desc{};
    desc.size = size;
    desc.mappedAtCreation = false;
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

    Ref<Buffer> buffer = newref<Buffer>();
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

void Buffer::update(View<uint8_t> view, size_t offset)
{
    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueWriteBuffer(Renderer::get().m_queue, m_buffer, static_cast<uint64_t>(offset), view.data(), view.size());
}

Texture::~Texture()
{
    if (!m_external)
    {
        wgpuTextureViewRelease(m_view);
        wgpuTextureRelease(m_texture);
    }
}

Result<Ref<Texture>> Texture::create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage, WGPUTextureViewDimension dimension, uint32_t layers, uint32_t mip_level)
{
    WGPUTextureDescriptor desc{};
    desc.usage = usage;
    desc.dimension = convert_texture_dimension(dimension);
    desc.size = WGPUExtent3D{.width = width, .height = height, .depthOrArrayLayers = layers == 0 ? 1 : layers};
    desc.format = format;
    desc.viewFormatCount = 1;
    desc.viewFormats = &format;
    desc.mipLevelCount = mip_level;
    desc.sampleCount = 1;

    if (mip_level > 1)
    {
        desc.usage |= WGPUTextureUsage_StorageBinding;
    }

    WGPUTexture texture = wgpuDeviceCreateTexture(Renderer::get().m_device, &desc);
    if (!texture)
        return Error(ErrorKind::OutOfDeviceMemory);

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = format;
    view_desc.dimension = dimension;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = mip_level;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = layers == 0 ? 1 : layers;

    WGPUTextureView view = wgpuTextureCreateView(texture, &view_desc);
    if (!view)
        return Error(ErrorKind::OutOfDeviceMemory);

    Ref<Texture> tex = newref<Texture>();
    tex->m_texture = texture;
    tex->m_view = view;
    tex->m_width = width;
    tex->m_height = height;
    tex->m_layers = layers;
    tex->m_mip_level = mip_level;
    tex->m_format = format;

    return tex;
}

Ref<Texture> Texture::create_from_handle(WGPUTexture texture, WGPUTextureView view)
{
    Ref<Texture> tex = newref<Texture>();
    tex->m_texture = texture;
    tex->m_view = view;
    tex->m_width = wgpuTextureGetWidth(texture);
    tex->m_height = wgpuTextureGetHeight(texture);
    tex->m_layers = wgpuTextureGetDepthOrArrayLayers(texture);
    tex->m_mip_level = wgpuTextureGetMipLevelCount(texture);
    tex->m_format = wgpuTextureGetFormat(texture);
    tex->m_external = true;
    return tex;
}

Result<Ref<Texture>> Texture::load(const StringView& path)
{
    File file = TRY(Filesystem::open_file(path));

    LocalVector<char> buffer;
    TRY(file.reader().read_to_buffer(buffer));
    file.close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);
    if (data == nullptr)
        return Error(ErrorKind::ReadFailure);

    Ref<Texture> texture = TRY(Texture::create(w, h, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureViewDimension_2D, 1, 1));
    texture->update(View((uint8_t *)data, w * h * 4));

    stbi_image_free(data);

    return texture;
}

void Texture::update(View<uint8_t> view, uint32_t layer)
{
    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());

#ifndef __platform_web
    WGPUTexelCopyTextureInfo copy_info{};
    copy_info.texture = m_texture;
    copy_info.aspect = WGPUTextureAspect_All;
    copy_info.origin.x = 0;
    copy_info.origin.y = 0;
    copy_info.origin.z = layer;
    copy_info.mipLevel = 0;

    WGPUTexelCopyBufferLayout layout{};
    layout.bytesPerRow = size_of(m_format) * m_width;
    layout.rowsPerImage = m_height;
    layout.offset = 0;

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

Result<Ref<Mesh>> Mesh::create_from_data(const View<uint8_t>& indices, const View<glm::vec3>& positions, const View<glm::vec3>& normals, const View<uint8_t>& uvs, WGPUIndexFormat index_type, UVType uv_type)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    Ref<Buffer> index_buffer = TRY(Buffer::create(indices.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index));
    index_buffer->update(indices.as_bytes());

    Ref<Buffer> vertex_buffer = TRY(Buffer::create(positions.size() * sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    vertex_buffer->update(positions.as_bytes());

    Ref<Buffer> uv_buffer = TRY(Buffer::create(uvs.size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    uv_buffer->update(uvs.as_bytes());

    Ref<Buffer> normal_buffer;
    if (normals.size() > 0)
    {
        normal_buffer = TRY(Buffer::create(normals.size() * sizeof(glm::vec3), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
        normal_buffer->update(normals.as_bytes());
    }

    return newref<Mesh>(vertex_count, index_type, uv_type, index_buffer, vertex_buffer, normal_buffer, uv_buffer);
}

static WGPUShaderModule create_shader_module(const Ref<Shader>& shader)
{
    WGPUShaderModuleDescriptor module_desc{};
    module_desc.label = WGPU_STRING_VIEW_INIT;

#ifndef __platform_web
    WGPUShaderSourceWGSL wgsl_source{};
    wgsl_source.chain = {.next = nullptr, .sType = WGPUSType_ShaderSourceWGSL};
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
    if (m_bind_group)
        wgpuBindGroupRelease(m_bind_group);

    for (const auto& [_, pipeline] : m_pipelines)
        wgpuRenderPipelineRelease(pipeline);
}

Result<Ref<Material>> Material::create(const Ref<Shader>& shader, MaterialFlags flags, WGPUCullMode cull_mode, UVType uv_type, Instance instance)
{
    Ref<Material> material = newref<Material>();
    material->m_shader = shader;
    material->m_flags = flags;
    material->m_cull_mode = cull_mode;
    material->m_uv_type = uv_type;
    material->m_attributes = instance.attribs;
    material->m_instance_stride = instance.stride;
    return material;
}

void Material::set_param(const StringView& name, const Ref<Texture>& texture)
{
    Option<Binding> binding_result = m_shader->get_binding(name);
    ASSERT_V(binding_result.has_value(), "Invalid parameter name `{}`", name.data());
    ASSERT_V(!texture.is_null(), "Texture specified for {} is null", name);

    m_caches.put(name, MaterialParamCache{.kind = binding_result.value().kind, .texture = texture});
    m_dirty = true;
}

void Material::set_param(const StringView& name, const Ref<Buffer>& buffer)
{
    Option<Binding> binding_result = m_shader->get_binding(name);
    ERR_COND_VR(buffer.is_null(), "Buffer specified for {} is null", name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    m_caches.put(name, MaterialParamCache{.kind = BindingKind::UniformBuffer, .buffer = buffer});
    m_dirty = true;
}

const MaterialParamCache& Material::get_param(const StringView& name) const
{
    ASSERT_V(m_caches.contains(name), "Cache missing {}", name);
    return *m_caches.get_ptr(name).value();
}

WGPUBindGroup Material::get_bind_group()
{
    if (m_dirty)
        create_bind_group();
    return m_bind_group;
}

WGPURenderPipeline Material::get_pipeline(const RenderPass& pass)
{
    PipelineKey key(pass.textures, pass.depth);
    Option<WGPURenderPipeline> p = m_pipelines.get(key);
    if (p.has_value())
        return p.value();

    WGPURenderPipeline pipeline = create_pipeline(pass);
    m_pipelines.put(key, pipeline);
    return pipeline;
}

void Material::create_bind_group()
{
    if (m_bind_group != nullptr)
        wgpuBindGroupRelease(m_bind_group);

    LocalVector<WGPUBindGroupEntry> entries;
    entries.reserve(m_shader->get_bindings().size());

    for (const auto& [_, name, binding] : m_shader->get_bindings())
    {
        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            const MaterialParamCache& cache = get_param(name);

            WGPUBindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.textureView = cache.texture->handle_view();

            entries.append(entry);

            SamplerDescriptor sampler = m_shader->get_sampler(name);

            WGPUSampler sampler_result = Renderer::get().get_sampler(sampler);
            ERR_COND_B(sampler_result == nullptr, "Unable to create a sampler");

            WGPUBindGroupEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.sampler = sampler_result;

            entries.append(sampler_entry);
        }
        break;
        case BindingKind::StorageTexture:
        {
            const MaterialParamCache& cache = get_param(name);

            WGPUBindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.textureView = cache.texture->handle_view();

            entries.append(entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            const MaterialParamCache& cache = get_param(name);
            ASSERT(cache.buffer->flags() & WGPUBufferUsage_Uniform, "Missing Uniform flag on buffer for param `{}`", name);

            WGPUBindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.buffer = cache.buffer->handle();
            entry.offset = 0;
            entry.size = cache.buffer->size();

            entries.append(entry);
        }
        break;
        case BindingKind::StorageBuffer:
        {
            const MaterialParamCache& cache = get_param(name);
            ASSERT(cache.buffer->flags() & WGPUBufferUsage_Storage, "Missing Storage flag on buffer for param `{}`", name);

            WGPUBindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.buffer = cache.buffer->handle();
            entry.offset = 0;
            entry.size = cache.buffer->size();

            entries.append(entry);
        }
        break;
        }
    }

    WGPUBindGroupDescriptor desc{};
    desc.nextInChain = nullptr;
    desc.layout = m_shader->get_bind_group_layout();
    desc.entries = entries.data();
    desc.entryCount = entries.size();

    m_bind_group = wgpuDeviceCreateBindGroup(Renderer::get().device(), &desc);
    ERR_COND_R(m_bind_group == nullptr, "Invalid bind group");

    m_dirty = false;
}

WGPURenderPipeline Material::create_pipeline(const RenderPass& pass)
{
    LocalVector<WGPUVertexBufferLayout> buffers;
    buffers.reserve(3 + m_attributes.size());

    uint32_t attrib_index = 0;

    WGPUVertexAttribute vertex_attrib{};
    if (!m_flags.has_all(MaterialFlagBits::NoPosition))
    {
        vertex_attrib.format = WGPUVertexFormat_Float32x3;
        vertex_attrib.offset = 0;
        vertex_attrib.shaderLocation = attrib_index++;
        buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib});
    }

    WGPUVertexAttribute normal_attrib{};
    if (!m_flags.has_any(MaterialFlagBits::NoNormal))
    {
        normal_attrib.format = WGPUVertexFormat_Float32x3;
        normal_attrib.offset = 0;
        normal_attrib.shaderLocation = attrib_index++;
        buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib});
    }

    WGPUVertexAttribute uv_attrib{};
    if (!m_flags.has_any(MaterialFlagBits::NoUV))
    {
        if (m_uv_type == UVType::UV)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x2;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec2), .attributeCount = 1, .attributes = &uv_attrib});
        }
        else if (m_uv_type == UVType::UVT)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x3;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &uv_attrib});
        }
    }

    LocalVector<WGPUVertexAttribute> attributes;
    attributes.reserve(m_attributes.size());
    for (uint32_t i = 0; i < m_attributes.size(); i++)
    {
        InstanceAttribute attrib = m_attributes[i];
        attributes.append(WGPUVertexAttribute{.nextInChain = nullptr, .format = attrib.format, .offset = attrib.offset, .shaderLocation = attrib_index + i});
    }
    if (m_attributes.size() > 0)
    {
        buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Instance, .arrayStride = m_instance_stride, .attributeCount = attributes.size(), .attributes = attributes.data()});
    }

    WGPUShaderModule module = create_shader_module(m_shader);
    ERR_COND_R(module == nullptr, "Unable to compile shader", nullptr);

    WGPUVertexState vertex_state{};
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

    Vector<WGPUColorTargetState> color_states;
    color_states.reserve(pass.textures.size());
    for (const RenderTarget& target : pass.textures)
        color_states.append(WGPUColorTargetState{.nextInChain = nullptr, .format = target.format, .blend = target.blending ? &blend_state : nullptr, .writeMask = WGPUColorWriteMask_All});

    WGPUFragmentState fragment_state{};
    fragment_state.targets = color_states.data();
    fragment_state.targetCount = color_states.size();
    fragment_state.entryPoint = WGPU_STRING_VIEW("fragment_main");
    fragment_state.module = module;

    WGPUDepthStencilState depth_state{};

    if (pass.depth.has_value())
    {
        depth_state.format = pass.depth.value().format;
        depth_state.depthWriteEnabled = WGPUOptionalBool_True;
        depth_state.depthCompare = WGPUCompareFunction_LessEqual;
    }

    WGPUPrimitiveState primitive_state{};
    primitive_state.cullMode = m_cull_mode;
    primitive_state.frontFace = WGPUFrontFace_CCW; // FIXME
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPURenderPipelineDescriptor desc{};
    desc.layout = m_shader->get_pipeline_layout();
    desc.primitive = primitive_state;
    desc.vertex = vertex_state;
    desc.fragment = color_states.size() == 0 ? nullptr : &fragment_state;
    desc.depthStencil = pass.depth.has_value() ? &depth_state : nullptr;
    desc.multisample = WGPUMultisampleState{.nextInChain = nullptr, .count = 1, .mask = 0xFFFFFFFF, .alphaToCoverageEnabled = false};

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(Renderer::get().device(), &desc);
    ERR_COND_R(pipeline == nullptr, "Failed to compile the render pipeline", nullptr);

    wgpuShaderModuleRelease(module);

    return pipeline;
}

WGPUSampler SamplerCache::get(const SamplerDescriptor& desc)
{
    Option<WGPUSampler> sampler_opt = m_samplers.get(desc);
    if (sampler_opt.has_value())
        return sampler_opt.value();

    WGPUSamplerDescriptor d{};
    d.magFilter = desc.mag_filter;
    d.minFilter = desc.min_filter;
    d.addressModeU = desc.address_mode.u;
    d.addressModeV = desc.address_mode.v;
    d.addressModeW = desc.address_mode.w;
    d.maxAnisotropy = 1;
    d.compare = desc.compare;

    WGPUSampler sampler = wgpuDeviceCreateSampler(Renderer::get().device(), &d);
    m_samplers.put(desc, sampler);

    return sampler;
}

void SamplerCache::clear()
{
    for (auto [_, sampler] : m_samplers)
        wgpuSamplerRelease(sampler);
}

Renderer::Renderer()
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

static Result<Ref<Mesh>> create_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    Array<uint16_t, 36> indices{
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

    Array<glm::vec3, 24> vertices{
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

    Array<glm::vec2, 24> uvs{
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

    Array<glm::vec3, 24> normals{
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

    return Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16);
}

#define SHADOWMAP_RESOLUTION 2048

Result<void> Renderer::init(const Window& window, InitFlags flags)
{
    singleton = this;

#ifndef __platform_web
    WGPUInstanceDescriptor instance_desc{};

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
    };

    WGPUDeviceDescriptor device_desc{};
    device_desc.nextInChain = nullptr;
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
            String s;
            s.append(message.data, message.length);
            println("{}", s);
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

    // const WGPUPipelineStatisticName pipeline_statistics[]{
    //     WGPUPipelineStatisticName_ClipperInvocations,
    // };

    // WGPUQuerySetDescriptorExtras query_set_extras{};
    // query_set_extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_QuerySetDescriptorExtras};
    // query_set_extras.pipelineStatisticCount = sizeof(pipeline_statistics) / sizeof(WGPUPipelineStatisticName);
    // query_set_extras.pipelineStatistics = pipeline_statistics;
    // WGPUQuerySetDescriptor query_set_desc{};
    // query_set_desc.nextInChain = &query_set_extras.chain;
    // query_set_desc.type = (WGPUQueryType)WGPUNativeQueryType_PipelineStatistics;
    // query_set_desc.count = 1;
    // m_pipeline_query_set = wgpuDeviceCreateQuerySet(m_device, &query_set_desc);

    // m_pipeline_query_set_buffer = TRY(Buffer::create(sizeof(uint64_t), WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc));

    m_camera_buffer = TRY(Buffer::create(sizeof(CameraUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_world_buffer = TRY(Buffer::create(sizeof(WorldUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_env_2d_buffer = TRY(Buffer::create(sizeof(glm::mat4), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_sky_buffer = TRY(Buffer::create(sizeof(SkyUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    m_shadowmap_buffer = TRY(Buffer::create(sizeof(ShadowmapCameraUniforms), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    m_shadowmap_texture = TRY(Texture::create(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));

    m_color_rect_shader = TRY(Shader::load_from_path("assets/shaders/ui/color_rect.wgsl"));
    m_color_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_color_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_color_rect_shader->create_bind_group_layout();

    m_texture_rect_shader = TRY(Shader::load_from_path("assets/shaders/ui/texture_rect.wgsl"));
    m_texture_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_texture_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_texture_rect_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D)); // binding = 3 is the sampler
    m_texture_rect_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_texture_rect_shader->create_bind_group_layout();

    m_text_shader = TRY(Shader::load_from_path("assets/shaders/ui/text.wgsl"));
    m_text_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_text_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_text_shader->set_binding("bitmap", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_text_shader->create_bind_group_layout();

    m_chunk_shader = TRY(Shader::load_from_path("assets/shaders/chunk.wgsl"));
    m_chunk_shader->set_binding("camera", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_chunk_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read, WGPUTextureViewDimension_2DArray));
    m_chunk_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_chunk_shader->create_bind_group_layout();

    m_water_shader = TRY(Shader::load_from_path("assets/shaders/water.wgsl"));
    m_water_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_water_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_water_shader->set_binding("camera", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_water_shader->create_bind_group_layout();

    m_ssao_shader = TRY(Shader::load_from_path("assets/shaders/ssao.wgsl"));
    m_ssao_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read));
    m_ssao_shader->set_binding("camera", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read));
    m_ssao_shader->set_binding("position_buffer", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_ssao_shader->set_sampler("position_buffer", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_ssao_shader->set_binding("normal_buffer", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_ssao_shader->set_sampler("normal_buffer", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_ssao_shader->set_binding("noise_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 6, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_ssao_shader->set_sampler("noise_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_ssao_shader->create_bind_group_layout();

    m_sky_shader = TRY(Shader::load_from_path("assets/shaders/sky.wgsl"));
    m_sky_shader->set_binding("albedo_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_sky_shader->set_binding("sky", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read));
    m_sky_shader->create_bind_group_layout();

    m_shading_shader = TRY(Shader::load_from_path("assets/shaders/shading.wgsl"));
    m_shading_shader->set_binding("albedo_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_shading_shader->set_binding("position_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("position_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("normal_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("normal_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("ssao_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 6, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("ssao_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("depth_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 8, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("depth_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("shadowmap_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 10, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("shadowmap_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("worldpos_texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 12, BindingAccess::Read, WGPUTextureViewDimension_2D, WGPUSamplerBindingType_NonFiltering));
    m_shading_shader->set_sampler("worldpos_texture", SamplerDescriptor(WGPUFilterMode_Undefined, WGPUFilterMode_Undefined));
    m_shading_shader->set_binding("camera", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Fragment, 0, 14, BindingAccess::Read));
    m_shading_shader->set_binding("world", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Fragment, 0, 15, BindingAccess::Read));
    m_shading_shader->create_bind_group_layout();

    m_preview_block_shader = TRY(Shader::load_from_path("assets/shaders/block_preview.wgsl"));
    m_preview_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_preview_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read, WGPUTextureViewDimension_2DArray)); // binding = 3 is the sampler
    m_preview_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_preview_block_shader->create_bind_group_layout();

    m_shadowmap_shader = TRY(Shader::load_from_path("assets/shaders/shadowmap.wgsl"));
    m_shadowmap_shader->set_binding("camera", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_shadowmap_shader->create_bind_group_layout();

    m_missing_texture = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding));
    m_missing_texture->update(View(missing_texture_data, 16 * 16).as_bytes());

    // Create resources for SSAO
    std::uniform_real_distribution<float> random_floats(0.0, 1.0);
    std::default_random_engine generator;

    Array<glm::vec4, 64> ssao_kernel;
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
    m_ssao_uniform_buffer->update(View(SSAOUniforms(ssao_kernel)).as_bytes());

    Array<glm::vec4, 16> ssao_noise;
    for (size_t i = 0; i < 16; i++)
    {
        const glm::vec4 noise(random_floats(generator) * 2.0 - 1.0, random_floats(generator) * 2.0 - 1.0, 0, 0);
        ssao_noise[i] = noise;
    }

    m_ssao_noise_texture = TRY(Texture::create(4, 4, WGPUTextureFormat_RGBA32Float, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst));
    m_ssao_noise_texture->update(View(ssao_noise).as_bytes());

    // m_model_shader = TRY(Shader::load(model_shader_source));
    // m_model_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    // m_model_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    // m_model_shader->set_binding("global_model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    // m_model_shader->set_binding("uvs", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 3, BindingAccess::Read));
    // m_model_shader->set_binding("texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, WGPUTextureViewDimension_2D));
    // m_model_shader->set_sampler("texture", SamplerDescriptor(WGPUFilterMode_Nearest, WGPUFilterMode_Nearest));
    // m_model_shader->create_bind_group_layout();

    m_cube_mesh = TRY(create_cube_mesh());

    // m_item_block_shader = TRY(Shader::load(item_block_shader_source));
    // m_item_block_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    // m_item_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    // m_item_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2DArray)); // binding = 3 is the sampler
    // m_item_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    // m_item_block_shader->create_bind_group_layout();

    Engine::get().registry().register_all();          // TODO: I dont really like having this here but it needs to be before calling `get_texture_array` and after initializing WebGPU.
    EXPECT(Engine::get().registry().post_register()); //       Maybe split this function in two (initializing/creating resources).

    Vector<InstanceAttribute> attributes;
    attributes.append(InstanceAttribute{.offset = 0, .format = WGPUVertexFormat_Float32x3});
    m_chunk_material = TRY(Material::create(m_chunk_shader, MaterialFlagBits::None, WGPUCullMode_Back, UVType::UVT, Instance(attributes, sizeof(glm::vec3))));
    m_chunk_material->set_param("camera", m_camera_buffer);
    m_chunk_material->set_param("images", Engine::get().registry().get_texture_array());

    m_water_material = TRY(Material::create(m_water_shader, MaterialFlagBits::Transparency, WGPUCullMode_Back, UVType::UV, Instance(attributes, sizeof(glm::vec3))));
    m_water_material->set_param("image", Engine::get().registry().create_texture("assets/textures/water.png"));
    m_water_material->set_param("camera", m_camera_buffer);

    m_ssao_material = TRY(Material::create(m_ssao_shader, MaterialFlagBits::NoData, WGPUCullMode_None, UVType::UV));
    m_ssao_material->set_param("uniforms", m_ssao_uniform_buffer);
    m_ssao_material->set_param("camera", m_camera_buffer);
    m_ssao_material->set_param("noise_texture", m_ssao_noise_texture);

    m_sky_material = TRY(Material::create(m_sky_shader, MaterialFlagBits::NoData, WGPUCullMode_None, UVType::UV));
    m_sky_material->set_param("sky", m_sky_buffer);

    m_shading_material = TRY(Material::create(m_shading_shader, MaterialFlagBits::NoData, WGPUCullMode_None, UVType::UV));
    m_shading_material->set_param("shadowmap_texture", m_shadowmap_texture);
    m_shading_material->set_param("camera", m_camera_buffer);
    m_shading_material->set_param("world", m_world_buffer);

    m_shadowmap_material = TRY(Material::create(m_shadowmap_shader, MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_Front, UVType::UV, Instance(attributes, sizeof(glm::vec3))));
    m_shadowmap_material->set_param("camera", m_shadowmap_buffer);

    // m_shadowmap_uniforms.view_matrix = glm::lookAt(glm::vec3(-10.0f, 85.0f, 0.0f),
    //                                                glm::vec3(0.0f, 0.0f, 0.0f),
    //                                                glm::vec3(0.0f, 1.0f, 0.0f));
    // m_shadowmap_uniforms.projection_matrix = glm::ortho(-10.0, 10.0, -10.0, 10.0);
    // m_shadowmap_buffer->update(View(m_shadowmap_uniforms).as_bytes());

    Array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};
    Array<glm::vec3, 4> vertices{
        glm::vec3(-0.5, -0.5, 0.1),
        glm::vec3(+0.5, -0.5, 0.1),
        glm::vec3(+0.5, +0.5, 0.1),
        glm::vec3(-0.5, +0.5, 0.1),
    };
    Array<glm::vec2, 4> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };
    m_square_mesh = TRY(Mesh::create_from_data(View(indices).as_bytes(), vertices, {}, View(uvs).as_bytes(), WGPUIndexFormat_Uint16));

    configure_surface(window.size().width, window.size().height, VSync::On);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForOther(window.get_window_ptr());

    ImGui_ImplWGPU_InitInfo init_info{};
    init_info.Device = m_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = m_surface_format;
    init_info.DepthStencilFormat = WGPUTextureFormat_Depth32Float;
    ImGui_ImplWGPU_Init(&init_info);

    return Result<void>();
}

void Renderer::configure_surface(size_t width, size_t height, VSync vsync)
{
    Extent2D surface_extent(width, height);

#ifndef __platform_web
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);

    WGPUSurfaceConfiguration config{};
    config.device = m_device;
    config.format = WGPUTextureFormat_BGRA8Unorm; // capabilities.formats[0];
    config.usage = capabilities.usages;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = vsync == VSync::On ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    config.alphaMode = capabilities.alphaModes[0];
#else
    WGPUSurfaceConfiguration config{};
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

    m_albedo_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));
    m_position_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_RGBA32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));
    m_normal_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_RGBA32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));
    m_depth_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));
    m_worldpos_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_RGBA32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));

    m_ssao_buffer = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_R32Float, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding));
    m_ssao_material->set_param("position_buffer", m_position_buffer);
    m_ssao_material->set_param("normal_buffer", m_normal_buffer);

    m_shading_material->set_param("albedo_texture", m_albedo_buffer);
    m_shading_material->set_param("position_texture", m_position_buffer);
    m_shading_material->set_param("normal_texture", m_normal_buffer);
    m_shading_material->set_param("ssao_texture", m_ssao_buffer);
    m_shading_material->set_param("depth_texture", m_depth_buffer);
    m_shading_material->set_param("worldpos_texture", m_worldpos_buffer);

    m_sky_material->set_param("albedo_texture", m_albedo_buffer);

    float ratio = float(width) / float(height);
    glm::mat4 ortho_matrix = glm::ortho(-1.0f * ratio, 1.0f * ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    m_env_2d_buffer->update(View(ortho_matrix).as_bytes());
}

void Renderer::draw_legacy(std::function<void()> f)
{
    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", surface_texture.status);
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
    depth_attach.view = m_depth_buffer->handle_view();
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
}

void Renderer::draw(const Ref<World>& world)
{
    ZoneScoped;

    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", surface_texture.status);
    // TODO: if status is WGPUSurfaceGetCurrentTextureStatus_Outdated or WGPUSurfaceGetCurrentTextureStatus_Timeout

    WGPUTextureView surface_view = wgpuTextureCreateView(surface_texture.texture, nullptr);
    ERR_COND_R(surface_view == nullptr, "Cannot acquire a swapchain image view");

    Ref<Texture> surface_tex = Texture::create_from_handle(surface_texture.texture, surface_view);

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    // Update environment data
    m_camera_buffer->update(View(CameraUniforms(world->get_active_camera()->get_view_matrix(), world->get_active_camera()->get_inv_view_matrix(), world->get_active_camera()->get_projection_matrix())).as_bytes());

    m_shadowmap_uniforms.view_projection = glm::ortho(10.0f, -10.0f, -10.0f, 10.0f, 1.0f, 100.0f) *
                                           glm::translate(glm::mat4(1.0), -world->get_active_camera()->get_global_transform().position());
    m_shadowmap_buffer->update(View(m_shadowmap_uniforms).as_bytes());

    const glm::vec3 position = world->get_active_camera()->get_global_transform().position();
    println("[ {} {} {} ]", position.x, position.y, position.z);

    m_world_uniforms.light_view_projection = m_shadowmap_uniforms.view_projection;
    m_world_buffer->update(View(m_world_uniforms).as_bytes());

    WGPURenderPassDepthStencilAttachment depth_buffer_attach{};
    depth_buffer_attach.depthClearValue = 1.0;
    depth_buffer_attach.depthLoadOp = WGPULoadOp_Clear;
    depth_buffer_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_buffer_attach.view = m_depth_buffer->handle_view();

    WGPURenderPassDepthStencilAttachment depth_buffer_load_attach{};
    depth_buffer_load_attach.depthClearValue = 1.0;
    depth_buffer_load_attach.depthLoadOp = WGPULoadOp_Load;
    depth_buffer_load_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_buffer_load_attach.view = m_depth_buffer->handle_view();

    // Depth prepass
    // WGPURenderPassDescriptor depth_pass{};
    // depth_pass.label = WGPU_STRING_VIEW("Depth prepass");
    // depth_pass.depthStencilAttachment = &depth_buffer_attach;

    // WGPURenderPassEncoder depth_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &depth_pass);
    // draw_world(world, RenderPass(depth_encoder, RenderTarget(m_depth_buffer->format()), Vector<RenderTarget>()));
    // wgpuRenderPassEncoderEnd(depth_encoder);
    // wgpuRenderPassEncoderRelease(depth_encoder);

    // Main color pass
    // > Generate the G-Buffer from the world geometry.
    WGPURenderPassColorAttachment main_pass_color_attachs[4]{};
    main_pass_color_attachs[0].clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    main_pass_color_attachs[0].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    main_pass_color_attachs[0].view = m_albedo_buffer->handle_view();
    main_pass_color_attachs[0].loadOp = WGPULoadOp_Clear;
    main_pass_color_attachs[0].storeOp = WGPUStoreOp_Store;

    main_pass_color_attachs[1].clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    main_pass_color_attachs[1].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    main_pass_color_attachs[1].view = m_position_buffer->handle_view();
    main_pass_color_attachs[1].loadOp = WGPULoadOp_Clear;
    main_pass_color_attachs[1].storeOp = WGPUStoreOp_Store;

    main_pass_color_attachs[2].clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    main_pass_color_attachs[2].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    main_pass_color_attachs[2].view = m_normal_buffer->handle_view();
    main_pass_color_attachs[2].loadOp = WGPULoadOp_Clear;
    main_pass_color_attachs[2].storeOp = WGPUStoreOp_Store;

    main_pass_color_attachs[3].clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    main_pass_color_attachs[3].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    main_pass_color_attachs[3].view = m_worldpos_buffer->handle_view();
    main_pass_color_attachs[3].loadOp = WGPULoadOp_Clear;
    main_pass_color_attachs[3].storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor main_pass{};
    main_pass.label = WGPU_STRING_VIEW("G-Buffer Pass");
    main_pass.colorAttachmentCount = sizeof(main_pass_color_attachs) / sizeof(WGPURenderPassColorAttachment);
    main_pass.colorAttachments = main_pass_color_attachs;
    main_pass.depthStencilAttachment = &depth_buffer_attach;

    WGPURenderPassEncoder main_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &main_pass);
    draw_world(world, RenderPass(main_pass_encoder, RenderTarget(m_depth_buffer->format()), Vector<RenderTarget>::create(RenderTarget(m_albedo_buffer->format(), false), RenderTarget(m_position_buffer->format(), false), RenderTarget(m_normal_buffer->format(), false), RenderTarget(m_worldpos_buffer->format(), false))));
    wgpuRenderPassEncoderEnd(main_pass_encoder);
    wgpuRenderPassEncoderRelease(main_pass_encoder);

    // Water Pass
    Map<float, Ref<Chunk>> chunks;
    for (const auto& [pos, chunk] : world->get_dimension(0).get_chunks())
    {
        const glm::vec3 posf = glm::vec3(float(pos.x) * 16.0, 128.0, float(pos.z) * 16.0);
        const float distance = glm::length2(posf - world->get_active_camera()->get_global_transform().position());
        chunks.put(distance, chunk);
    }

    WGPURenderPassColorAttachment transparent_color_attach{};
    transparent_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    transparent_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    transparent_color_attach.loadOp = WGPULoadOp_Load;
    transparent_color_attach.storeOp = WGPUStoreOp_Store;
    transparent_color_attach.view = m_albedo_buffer->handle_view();

    WGPURenderPassDescriptor transparent_pass{};
    transparent_pass.label = WGPU_STRING_VIEW("Transparent Pass");
    transparent_pass.colorAttachmentCount = 1;
    transparent_pass.colorAttachments = &transparent_color_attach;
    transparent_pass.depthStencilAttachment = &depth_buffer_load_attach;

    WGPURenderPassEncoder transparent_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &transparent_pass);
    wgpuRenderPassEncoderSetPipeline(transparent_pass_encoder, m_water_material->get_pipeline(RenderPass(transparent_pass_encoder, RenderTarget(m_depth_buffer->format()), Vector<RenderTarget>::create(m_albedo_buffer->format()))));
    wgpuRenderPassEncoderSetBindGroup(transparent_pass_encoder, 0, m_water_material->get_bind_group(), 0, nullptr);

    for (ssize_t i = (ssize_t)world->get_dimension(0).get_chunks().size() - 1; i >= 0; i--)
    {
        const Ref<Chunk>& chunk = world->get_dimension(0).get_chunks().pairs()[i].value;
        ChunkPos pos = chunk->pos();
        AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                        .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

        if (!world->get_active_camera()->frustum().contains(aabb))
            continue;

        for (size_t slice_index = 0; slice_index < Chunk::slice_count; slice_index++)
        {
            const Chunk::Slice& slice = chunk->get_slices()[slice_index];
            const Ref<Mesh>& mesh = slice.water_mesh;

            if (mesh.is_null())
                continue;

            AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
                            .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)slice_index * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            if (!world->get_active_camera()->frustum().contains(aabb))
                continue;

            wgpuRenderPassEncoderSetIndexBuffer(transparent_pass_encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
            wgpuRenderPassEncoderSetVertexBuffer(transparent_pass_encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
            wgpuRenderPassEncoderSetVertexBuffer(transparent_pass_encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
            wgpuRenderPassEncoderSetVertexBuffer(transparent_pass_encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
            wgpuRenderPassEncoderSetVertexBuffer(transparent_pass_encoder, 3, chunk->get_chunk_instance_buffer()->handle(), sizeof(glm::vec3) * slice_index, sizeof(glm::vec3));
            wgpuRenderPassEncoderDrawIndexed(transparent_pass_encoder, mesh->vertex_count(), 1, 0, 0, 0);
        }
    }
    wgpuRenderPassEncoderEnd(transparent_pass_encoder);
    wgpuRenderPassEncoderRelease(transparent_pass_encoder);

    // Shadow Map
    WGPURenderPassDepthStencilAttachment shadowmap_depth_attach{};
    shadowmap_depth_attach.depthClearValue = 1.0;
    shadowmap_depth_attach.view = m_shadowmap_texture->handle_view();
    shadowmap_depth_attach.depthLoadOp = WGPULoadOp_Clear;
    shadowmap_depth_attach.depthStoreOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor shadowmap_pass{};
    shadowmap_pass.label = WGPU_STRING_VIEW("ShadowMap Pass");
    shadowmap_pass.depthStencilAttachment = &shadowmap_depth_attach;

    WGPURenderPassEncoder shadowmap_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &shadowmap_pass);
    draw_world(world, RenderPass(shadowmap_encoder, RenderTarget(m_shadowmap_texture->format()), {}), m_shadowmap_material, [](const RenderPass& pass, const Ref<Chunk>& chunk, size_t i, const Ref<Mesh>& mesh)
               {
                   wgpuRenderPassEncoderSetIndexBuffer(pass.encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
                   wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
                   wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 1, chunk->get_chunk_instance_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));
                   wgpuRenderPassEncoderDrawIndexed(pass.encoder, mesh->vertex_count(), 1, 0, 0, 0); });
    wgpuRenderPassEncoderEnd(shadowmap_encoder);
    wgpuRenderPassEncoderRelease(shadowmap_encoder);

    // Screen-Space Ambiant Occlusion
    WGPURenderPassColorAttachment ssao_buffer_attach{};
    ssao_buffer_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    ssao_buffer_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ssao_buffer_attach.view = m_ssao_buffer->handle_view();
    ssao_buffer_attach.loadOp = WGPULoadOp_Clear;
    ssao_buffer_attach.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor ssao_pass{};
    ssao_pass.colorAttachmentCount = 1;
    ssao_pass.colorAttachments = &ssao_buffer_attach;

    WGPURenderPassEncoder ssao_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &ssao_pass);
    draw_fullscreen(RenderPass(ssao_encoder, None, Vector<RenderTarget>::create(RenderTarget(m_ssao_buffer->format(), false))), m_ssao_material);
    wgpuRenderPassEncoderEnd(ssao_encoder);
    wgpuRenderPassEncoderRelease(ssao_encoder);

    // Sky
    WGPURenderPassColorAttachment sky_color_attach{};
    sky_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    sky_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    sky_color_attach.view = surface_view;
    sky_color_attach.loadOp = WGPULoadOp_Clear;
    sky_color_attach.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor sky_pass{};
    sky_pass.label = WGPU_STRING_VIEW("Sky Pass");
    sky_pass.colorAttachmentCount = 1;
    sky_pass.colorAttachments = &sky_color_attach;

    WGPURenderPassEncoder sky_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &sky_pass);
    draw_fullscreen(RenderPass(sky_encoder, None, Vector<RenderTarget>::create(m_surface_format)), m_sky_material);
    wgpuRenderPassEncoderEnd(sky_encoder);
    wgpuRenderPassEncoderRelease(sky_encoder);

    // Shading
    WGPURenderPassColorAttachment shading_color_attach{};
    shading_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    shading_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    shading_color_attach.view = surface_view;
    shading_color_attach.loadOp = WGPULoadOp_Load;
    shading_color_attach.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor shading_pass{};
    shading_pass.label = WGPU_STRING_VIEW("Shading Pass");
    shading_pass.colorAttachmentCount = 1;
    shading_pass.colorAttachments = &shading_color_attach;

    WGPURenderPassEncoder shading_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &shading_pass);
    draw_fullscreen(RenderPass(shading_encoder, None, Vector<RenderTarget>::create(m_surface_format)), m_shading_material);
    wgpuRenderPassEncoderEnd(shading_encoder);
    wgpuRenderPassEncoderRelease(shading_encoder);

    // UI Pass
    WGPURenderPassColorAttachment ui_color_attach{};
    ui_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    ui_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ui_color_attach.view = surface_view;
    ui_color_attach.loadOp = WGPULoadOp_Load;
    ui_color_attach.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor ui_pass{};
    ui_pass.label = WGPU_STRING_VIEW("UI pass");
    ui_pass.colorAttachmentCount = 1;
    ui_pass.colorAttachments = &ui_color_attach;
    ui_pass.depthStencilAttachment = &depth_buffer_attach;
    WGPURenderPassEncoder ui_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &ui_pass);
    for (Ref<Entity> entity : world->get_dimension(0).get_entities())
        entity->draw_ui(RenderPass(ui_encoder, RenderTarget(m_depth_buffer->format()), Vector<RenderTarget>::create(m_surface_format)));
    wgpuRenderPassEncoderEnd(ui_encoder);
    wgpuRenderPassEncoderRelease(ui_encoder);

    // wgpuCommandEncoderResolveQuerySet(encoder, m_pipeline_query_set, 0, 1, m_pipeline_query_set_buffer->handle(), 0);

    // Then submit everything to the GPU.
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

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
}

void Renderer::draw(const RenderPass& pass, Ref<Mesh> mesh, Ref<Material> material, const Ref<Buffer>& instance_buffer, size_t instance_count)
{
    wgpuRenderPassEncoderSetPipeline(pass.encoder, material->get_pipeline(pass));
    wgpuRenderPassEncoderSetBindGroup(pass.encoder, 0, material->get_bind_group(), 0, nullptr);
    wgpuRenderPassEncoderSetIndexBuffer(pass.encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());

    size_t buffer_index = 1;
    if (!material->flags().has_any(MaterialFlagBits::NoNormal))
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
    if (!material->flags().has_any(MaterialFlagBits::NoUV))
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

    if (!instance_buffer.is_null())
        wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, buffer_index++, instance_buffer->handle(), 0, instance_buffer->size());

    wgpuRenderPassEncoderDrawIndexed(pass.encoder, mesh->vertex_count(), instance_count, 0, 0, 0);
}

void Renderer::draw_fullscreen(const RenderPass& pass, Ref<Material> material)
{
    wgpuRenderPassEncoderSetPipeline(pass.encoder, material->get_pipeline(pass));
    wgpuRenderPassEncoderSetBindGroup(pass.encoder, 0, material->get_bind_group(), 0, nullptr);
    wgpuRenderPassEncoderDraw(pass.encoder, 4, 1, 0, 0);
}

void Renderer::draw_world(const Ref<World>& world, const RenderPass& pass)
{
    ZoneScoped;

    const Dimension& dim = world->get_dimension(0);
    const Ref<Camera> camera = world->get_active_camera();
    const glm::vec3 camera_pos = camera->get_position();
    WGPURenderPassEncoder encoder = pass.encoder;

    if (camera.is_null())
    {
        return;
    }

    WGPURenderPipeline pipeline = m_chunk_material->get_pipeline(pass);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_chunk_material->get_bind_group(), 0, nullptr);

    for (const auto& [key, chunk] : dim.get_chunks())
    {
        ChunkPos pos = chunk->pos();
        AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                        .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

        if (!camera->frustum().contains(aabb))
            continue;

        const Chunk::Slice *slices = chunk->get_slices();

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            const Chunk::Slice& slice = slices[i];

            if (slice.mesh.is_null())
                continue;

            ChunkPos pos = chunk->pos();
            AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
                            .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            if (!camera->frustum().contains(aabb))
                continue;

            const glm::vec4 p(float(pos.x) * 16.0 - camera_pos.x, float(i) * 16.0, float(pos.z) * 16.0 - camera_pos.z, 1.0);

            // chunk->get_chunk_instance_buffer()->update(View(glm::vec3(p)).as_bytes(), i * sizeof(glm::vec3));

            const Ref<Mesh>& mesh = slice.mesh;
            wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 3, chunk->get_chunk_instance_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));

            wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
        }
    }
}

void Renderer::draw_world(const Ref<World>& world, const RenderPass& pass, Ref<Material> material, std::function<void(const RenderPass& pass, const Ref<Chunk>&, size_t, const Ref<Mesh>&)> f)
{
    const Dimension& dim = world->get_dimension(0);
    const Ref<Camera> camera = world->get_active_camera();
    const glm::vec3 camera_pos = camera->get_position();

    WGPURenderPipeline pipeline = material->get_pipeline(pass);
    wgpuRenderPassEncoderSetPipeline(pass.encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(pass.encoder, 0, material->get_bind_group(), 0, nullptr);

    for (const auto& [key, chunk] : dim.get_chunks())
    {
        // ChunkPos pos = chunk->pos();
        // AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
        //                 .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

        // if (!camera->frustum().contains(aabb))
        //     continue;

        const Chunk::Slice *slices = chunk->get_slices();

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            const Chunk::Slice& slice = slices[i];

            if (slice.mesh.is_null())
                continue;

            // ChunkPos pos = chunk->pos();
            // AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
            //                 .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            // if (!camera->frustum().contains(aabb))
            //     continue;

            const Ref<Mesh>& mesh = slice.mesh;
            f(pass, chunk, i, mesh);
            // wgpuRenderPassEncoderSetIndexBuffer(pass.encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
            // wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
            // wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
            // wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
            // wgpuRenderPassEncoderSetVertexBuffer(pass.encoder, 3, chunk->get_chunk_instance_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));

            // wgpuRenderPassEncoderDrawIndexed(pass.encoder, mesh->vertex_count(), 1, 0, 0, 0);
        }
    }
}

void Renderer::set_fog(glm::vec4 color, float distance)
{
    m_world_uniforms.fog_color = color;
    m_world_uniforms.fog_distance = distance;
    m_world_buffer->update(View(m_world_uniforms).as_bytes());
}

void Renderer::set_sky(glm::vec4 color)
{
    m_sky_buffer->update(View(SkyUniforms(color)).as_bytes());
}

void Renderer::set_underwater(bool v)
{
    m_world_uniforms.underwater = v;
    m_world_buffer->update(View(m_world_uniforms).as_bytes());
}

View<uint8_t> Renderer::get_missing_texture_data() const
{
    return View((uint8_t *)missing_texture_data, 16 * 16 * sizeof(uint32_t));
}
