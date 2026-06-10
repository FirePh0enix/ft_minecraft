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
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include "webgpu/webgpu.h"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>

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

    if (visibility == BufferVisibility::GPUAndCPU)
        desc.usage |= WGPUBufferUsage_MapRead;

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

Ref<Texture> Texture::create_from_view(WGPUTextureView view)
{
    Ref<Texture> tex = newref<Texture>();
    tex->m_texture = nullptr;
    tex->m_view = view;
    tex->m_width = 0;
    tex->m_height = 0;
    tex->m_layers = 0;
    tex->m_mip_level = 0;
    tex->m_format = WGPUTextureFormat_Undefined;
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

Material::~Material()
{
    if (m_bind_group)
        wgpuBindGroupRelease(m_bind_group);
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

    // TODO: Check dimensions.

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

Result<WGPURenderPipeline> PipelineCache::get(const Key& key)
{
    Option<WGPURenderPipeline> pipeline_opt = m_pipelines.get(key);
    if (pipeline_opt.has_value())
        return pipeline_opt.value();

    LocalVector<WGPUVertexBufferLayout> buffers;
    buffers.reserve(3 + key.attributes.size());

    uint32_t attrib_index = 0;

    WGPUVertexAttribute vertex_attrib{};
    vertex_attrib.format = WGPUVertexFormat_Float32x3;
    vertex_attrib.offset = 0;
    vertex_attrib.shaderLocation = attrib_index++;
    buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib});

    WGPUVertexAttribute normal_attrib{};
    if (!key.flags.has_any(MaterialFlagBits::NoNormal))
    {
        normal_attrib.format = WGPUVertexFormat_Float32x3;
        normal_attrib.offset = 0;
        normal_attrib.shaderLocation = attrib_index++;
        buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib});
    }

    WGPUVertexAttribute uv_attrib{};
    if (!key.flags.has_any(MaterialFlagBits::NoUV))
    {
        if (key.uv_type == UVType::UV)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x2;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec2), .attributeCount = 1, .attributes = &uv_attrib});
        }
        else if (key.uv_type == UVType::UVT)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x3;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &uv_attrib});
        }
    }

    LocalVector<WGPUVertexAttribute> attributes;
    attributes.reserve(key.attributes.size());
    for (uint32_t i = 0; i < key.attributes.size(); i++)
    {
        InstanceAttribute attrib = key.attributes.get_unchecked(i);
        attributes.append(WGPUVertexAttribute{.nextInChain = nullptr, .format = attrib.format, .offset = attrib.offset, .shaderLocation = attrib_index + i});
    }
    if (key.attributes.size() > 0)
    {
        buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Instance, .arrayStride = key.instance_stride, .attributeCount = attributes.size(), .attributes = attributes.data()});
    }

    WGPUShaderModule module = create_shader_module(key.shader);
    ERR_COND_R(module == nullptr, "Unable to compile shader", Error(ErrorKind::BadDriver));

    String vertex_ep = key.shader->get_entry_point(WGPUShaderStage_Vertex);

    WGPUVertexState vertex_state{};
    vertex_state.buffers = buffers.data();
    vertex_state.bufferCount = buffers.size();
    vertex_state.entryPoint = WGPU_STRING_VIEW(vertex_ep.data());
    vertex_state.module = module;

    WGPUBlendState blend_state{};

    if (!key.flags.has_any(MaterialFlagBits::Transparency))
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
    if (key.has_color_attach)
        color_states.append(WGPUColorTargetState{.nextInChain = nullptr, .format = key.color_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All});

    String fragment_ep = key.shader->get_entry_point(WGPUShaderStage_Fragment);

    WGPUFragmentState fragment_state{};
    fragment_state.targets = color_states.data();
    fragment_state.targetCount = color_states.size();
    fragment_state.entryPoint = WGPU_STRING_VIEW(fragment_ep.data());
    fragment_state.module = module;

    WGPUDepthStencilState depth_state{};
    depth_state.format = key.depth_format;
    depth_state.depthWriteEnabled = WGPUOptionalBool_True;
    depth_state.depthCompare = WGPUCompareFunction_LessEqual;

    WGPUPrimitiveState primitive_state{};
    primitive_state.cullMode = key.cull_mode;
    primitive_state.frontFace = WGPUFrontFace_CCW; // FIXME
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPURenderPipelineDescriptor desc{};
    desc.layout = key.shader->get_pipeline_layout();
    desc.primitive = primitive_state;
    desc.vertex = vertex_state;
    desc.fragment = color_states.size() == 0 ? nullptr : &fragment_state;
    desc.depthStencil = key.has_depth_attach ? &depth_state : nullptr;
    desc.multisample = WGPUMultisampleState{.nextInChain = nullptr, .count = 1, .mask = 0xFFFFFFFF, .alphaToCoverageEnabled = false};

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(Renderer::get().device(), &desc);
    ERR_COND_R(pipeline == nullptr, "Failed to compile the render pipeline", Error(ErrorKind::BadDriver));

    wgpuShaderModuleRelease(module);

    m_pipelines.put(key, pipeline);
    return pipeline;
}

Result<WGPUComputePipeline> PipelineCache::get_compute(const ComputeKey& key)
{
    Option<WGPUComputePipeline> pipeline_opt = m_compute_pipelines.get(key);
    if (pipeline_opt.has_value())
        return pipeline_opt.value();

    WGPUShaderModule module = create_shader_module(key.shader);

    WGPUComputePipelineDescriptor desc{};
    desc.layout = key.shader->get_pipeline_layout();
    desc.compute.module = module;
    desc.compute.entryPoint = WGPU_STRING_VIEW("main");

    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(Renderer::get().device(), &desc);
    ERR_COND_R(pipeline == nullptr, "Failed to compile the compute pipeline", Error(ErrorKind::BadDriver));

    wgpuShaderModuleRelease(module);

    m_compute_pipelines.put(key, pipeline);
    return pipeline;
}

void PipelineCache::clear()
{
    for (auto [_, pipeline] : m_pipelines)
        wgpuRenderPipelineRelease(pipeline);
    for (auto [_, pipeline] : m_compute_pipelines)
        wgpuComputePipelineRelease(pipeline);
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

Result<void> Renderer::init(const Window& window, InitFlags flags)
{
    singleton = this;

#ifndef __platform_web
    WGPUInstanceDescriptor instance_desc{};

    // WGPUInstanceLayerSelection lsel{.chain = {.next = nullptr, .sType = WGPUSType_InstanceLayerSelection}, .instanceLayers = nullptr, .instanceLayerCount = 0};
    // const char *instance_layer[]{"VK_LAYER_KHRONOS_validation"};
    // lsel.instanceLayerCount = 1;
    // lsel.instanceLayers = instance_layer;

    // if (flags.has_any(InitFlagBits::Validation))
    // {
    //     instance_desc.nextInChain = &lsel.chain;
    // }

    (void)flags;

    const WGPUInstanceFeatureName features[]{
        WGPUInstanceFeatureName_TimedWaitAny,
    };
    instance_desc.requiredFeatureCount = sizeof(features) / sizeof(WGPUInstanceFeatureName);
    instance_desc.requiredFeatures = features;

    // WGPUInstanceExtras extras{};
    // extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_InstanceExtras};
    // extras.backends = WGPUInstanceBackend_Primary;

    // if (flags.has_any(InitFlagBits::Validation))
    //     extras.flags |= WGPUInstanceFlag_Validation;

    // instance_desc.nextInChain = &extras.chain;

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

    // TODO: Use this maybe ?
    // wgpuInstanceEnumerateAdapters(WGPUInstance instance, const WGPUInstanceEnumerateAdapterOptions *options, WGPUAdapter *adapters);

    const WGPUFeatureName required_features[] = {
        // WGPUFeatureName_BGRA8UnormStorage,
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
    if (!m_queue)
        return Error(ErrorKind::BadDriver);

    m_env_2d_buffer = TRY(Buffer::create(sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_sky_shader = TRY(Shader::load(sky_shader_source));
    m_sky_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_sky_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_sky_shader->create_bind_group_layout();

    m_sky_uniform_buffer = TRY(Buffer::create(sizeof(SkyUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_sky_material = TRY(Material::create(m_sky_shader, MaterialFlagBits::NoUV | MaterialFlagBits::NoNormal, WGPUCullMode_None, UVType::UV));
    m_sky_material->set_param("env", m_env_2d_buffer);
    m_sky_material->set_param("uniforms", m_sky_uniform_buffer);

    m_surface_shader = TRY(Shader::load(surface_shader_source));
    m_surface_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_surface_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_surface_shader->set_binding("render_target", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_surface_shader->create_bind_group_layout();

    m_surface_material = TRY(Material::create(m_surface_shader, MaterialFlagBits::NoNormal, WGPUCullMode_None, UVType::UV));
    m_surface_material->set_param("env", m_env_2d_buffer);
    m_surface_material->set_param("uniforms", m_sky_uniform_buffer);

    m_underwater_shader = TRY(Shader::load(underwater_shader_source));
    m_underwater_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_underwater_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_underwater_shader->set_binding("render_target", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_underwater_shader->create_bind_group_layout();

    m_underwater_material = TRY(Material::create(m_underwater_shader, MaterialFlagBits::NoNormal, WGPUCullMode_None, UVType::UV));
    m_underwater_material->set_param("env", m_env_2d_buffer);
    m_underwater_material->set_param("uniforms", m_sky_uniform_buffer);

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

    m_missing_texture = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding));
    m_missing_texture->update(View(missing_texture_data, 16 * 16).as_bytes());

    m_env_buffer = TRY(Buffer::create(sizeof(WorldEnvironment), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_voxel_shader = TRY(Shader::load(chunk_mesh_shader_source));
    m_voxel_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, WGPUTextureViewDimension_2DArray)); // binding = 1 is the sampler
    m_voxel_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_voxel_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_voxel_shader->create_bind_group_layout();

    m_water_shader = TRY(Shader::load(water_mesh_shader_source));
    m_water_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, WGPUTextureViewDimension_2D)); // binding = 1 is the sampler
    m_water_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_water_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_water_shader->create_bind_group_layout();

    m_model_shader = TRY(Shader::load(model_shader_source));
    m_model_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_model_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_model_shader->set_binding("global_model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_model_shader->set_binding("uvs", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 3, BindingAccess::Read));
    m_model_shader->set_binding("texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, WGPUTextureViewDimension_2D));
    m_model_shader->set_sampler("texture", SamplerDescriptor(WGPUFilterMode_Nearest, WGPUFilterMode_Nearest));
    m_model_shader->create_bind_group_layout();

    m_cube_mesh = TRY(create_cube_mesh());

    m_preview_block_shader = TRY(Shader::load(block_preview_shader_source));
    m_preview_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_preview_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read, WGPUTextureViewDimension_2DArray)); // binding = 3 is the sampler
    m_preview_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_preview_block_shader->create_bind_group_layout();

    m_item_block_shader = TRY(Shader::load(item_block_shader_source));
    m_item_block_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_item_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_item_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2DArray)); // binding = 3 is the sampler
    m_item_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_item_block_shader->create_bind_group_layout();

    Engine::get().registry().register_all();          // TODO: I dont really like having this here but it needs to be before calling `get_texture_array` and after initializing WebGPU.
    EXPECT(Engine::get().registry().post_register()); //       Maybe split this function in two (initializing/creating resources).

    Vector<InstanceAttribute> attributes;
    attributes.append(InstanceAttribute{.offset = 0, .format = WGPUVertexFormat_Float32x3});

    m_chunk_material = TRY(Material::create(m_voxel_shader, MaterialFlagBits::None, WGPUCullMode_Back, UVType::UVT, Instance(attributes, sizeof(glm::vec3))));
    m_chunk_material->set_param("images", Engine::get().registry().get_texture_array());
    m_chunk_material->set_param("env", Renderer::get().get_world_environment());

    m_water_material = TRY(Material::create(m_water_shader, MaterialFlagBits::Transparency, WGPUCullMode_Back, UVType::UV, Instance(attributes, sizeof(glm::vec3))));
    m_water_material->set_param("image", Engine::get().registry().create_texture("assets/textures/water.png"));
    m_water_material->set_param("env", Renderer::get().get_world_environment());

    m_simple_shader = TRY(Shader::load(simple_shape_source));
    m_simple_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_simple_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_simple_shader->create_bind_group_layout();

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

    m_color_rect_shader = TRY(Shader::load(color_rect_shader_source));
    m_color_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_color_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_color_rect_shader->create_bind_group_layout();

    m_texture_rect_shader = TRY(Shader::load(texture_rect_shader_source));
    m_texture_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_texture_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_texture_rect_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, WGPUTextureViewDimension_2D)); // binding = 3 is the sampler
    m_texture_rect_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_texture_rect_shader->create_bind_group_layout();

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

    m_depth_texture = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment));
    m_color_texture = EXPECT(Texture::create(m_surface_extent.width, m_surface_extent.height, m_color_format, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding));

    float ratio = float(width) / float(height);
    glm::mat4 ortho_matrix = glm::ortho(-1.0f * ratio, 1.0f * ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    m_env_2d_buffer->update(View(ortho_matrix).as_bytes());

    m_sky_uniforms.model_matrix = glm::scale(glm::mat4(1.0), glm::vec3(2.0 * ratio, 2.0, 1.0));
    m_sky_uniform_buffer->update(View(m_sky_uniforms).as_bytes());

    m_surface_material->set_param("render_target", m_color_texture);
    m_underwater_material->set_param("render_target", m_color_texture);
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
    rp.occlusionQuerySet = nullptr;
    rp.timestampWrites = nullptr;

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
    depth_attach.view = m_depth_texture->handle_view();
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

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    // Sky shader
    WGPURenderPassDescriptor sky_pass{};
    sky_pass.nextInChain = nullptr;
    sky_pass.label = WGPU_STRING_VIEW_INIT;
    sky_pass.timestampWrites = nullptr;
    sky_pass.occlusionQuerySet = nullptr;

    WGPURenderPassColorAttachment sky_color_attach{};
    sky_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    sky_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    sky_color_attach.loadOp = WGPULoadOp_Clear;
    sky_color_attach.storeOp = WGPUStoreOp_Store;
    sky_color_attach.view = m_color_texture->handle_view();

    sky_pass.colorAttachmentCount = 1;
    sky_pass.colorAttachments = &sky_color_attach;

    WGPURenderPassEncoder sky_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &sky_pass);
    wgpuRenderPassEncoderSetPipeline(sky_encoder, get_pipeline(m_sky_material, m_color_format, WGPUTextureFormat_Undefined));
    wgpuRenderPassEncoderSetBindGroup(sky_encoder, 0, m_sky_material->get_bind_group(), 0, nullptr);
    wgpuRenderPassEncoderSetIndexBuffer(sky_encoder, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->handle(), m_square_mesh->index_type(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(sky_encoder, 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->size());
    wgpuRenderPassEncoderDrawIndexed(sky_encoder, m_square_mesh->vertex_count(), 1, 0, 0, 0);
    wgpuRenderPassEncoderEnd(sky_encoder);
    wgpuRenderPassEncoderRelease(sky_encoder);

    // Main color pass
    WGPURenderPassDescriptor main_pass{};
    main_pass.nextInChain = nullptr;
    main_pass.label = WGPU_STRING_VIEW_INIT;
    main_pass.timestampWrites = nullptr;
    main_pass.occlusionQuerySet = nullptr;

    WGPURenderPassColorAttachment color_attach{};
    color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attach.loadOp = WGPULoadOp_Load;
    color_attach.storeOp = WGPUStoreOp_Store;
    color_attach.view = m_color_texture->handle_view();

    main_pass.colorAttachmentCount = 1;
    main_pass.colorAttachments = &color_attach;

    WGPURenderPassDepthStencilAttachment depth_attach{};
    depth_attach.depthClearValue = 1.0;
    depth_attach.depthLoadOp = WGPULoadOp_Clear;
    depth_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_attach.view = m_depth_texture->handle_view();
    main_pass.depthStencilAttachment = &depth_attach;

    WGPURenderPassEncoder render_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &main_pass);
    record_world(world, render_encoder);
    wgpuRenderPassEncoderEnd(render_encoder);
    wgpuRenderPassEncoderRelease(render_encoder);

    // Render to surface
    // > WebGPU does not support read write storage texture with the surface format.
    // > Apply the post processing effect here.
    WGPURenderPassDescriptor surface_pass{};
    surface_pass.nextInChain = nullptr;
    surface_pass.label = WGPU_STRING_VIEW_INIT;
    surface_pass.timestampWrites = nullptr;
    surface_pass.occlusionQuerySet = nullptr;
    surface_pass.depthStencilAttachment = nullptr;

    WGPURenderPassColorAttachment surface_color_attach{};
    surface_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    surface_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    surface_color_attach.loadOp = WGPULoadOp_Clear;
    surface_color_attach.storeOp = WGPUStoreOp_Store;
    surface_color_attach.view = surface_view;

    surface_pass.colorAttachmentCount = 1;
    surface_pass.colorAttachments = &surface_color_attach;

    if (m_underwater_effect)
    {
        WGPURenderPassEncoder surface_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &surface_pass);
        wgpuRenderPassEncoderSetPipeline(surface_encoder, get_pipeline(m_underwater_material, m_surface_format, WGPUTextureFormat_Undefined));
        wgpuRenderPassEncoderSetBindGroup(surface_encoder, 0, m_underwater_material->get_bind_group(), 0, nullptr);
        wgpuRenderPassEncoderSetIndexBuffer(surface_encoder, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->handle(), m_square_mesh->index_type(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->size());
        wgpuRenderPassEncoderSetVertexBuffer(surface_encoder, 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->size());
        wgpuRenderPassEncoderSetVertexBuffer(surface_encoder, 1, m_square_mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::UV)->size());
        wgpuRenderPassEncoderDrawIndexed(surface_encoder, m_square_mesh->vertex_count(), 1, 0, 0, 0);
        wgpuRenderPassEncoderEnd(surface_encoder);
        wgpuRenderPassEncoderRelease(surface_encoder);
    }
    else
    {
        WGPURenderPassEncoder surface_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &surface_pass);
        wgpuRenderPassEncoderSetPipeline(surface_encoder, get_pipeline(m_surface_material, m_surface_format, WGPUTextureFormat_Undefined));
        wgpuRenderPassEncoderSetBindGroup(surface_encoder, 0, m_surface_material->get_bind_group(), 0, nullptr);
        wgpuRenderPassEncoderSetIndexBuffer(surface_encoder, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->handle(), m_square_mesh->index_type(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Index)->size());
        wgpuRenderPassEncoderSetVertexBuffer(surface_encoder, 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::Position)->size());
        wgpuRenderPassEncoderSetVertexBuffer(surface_encoder, 1, m_square_mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, m_square_mesh->get_buffer(Mesh::BufferKind::UV)->size());
        wgpuRenderPassEncoderDrawIndexed(surface_encoder, m_square_mesh->vertex_count(), 1, 0, 0, 0);
        wgpuRenderPassEncoderEnd(surface_encoder);
        wgpuRenderPassEncoderRelease(surface_encoder);
    }

    // UI Pass
    // > Since we don't want to add our post processing effects to the UI we render it after. It can be done directly on the surface.
    WGPURenderPassDescriptor ui_pass{};
    ui_pass.nextInChain = nullptr;
    ui_pass.label = WGPU_STRING_VIEW_INIT;
    ui_pass.timestampWrites = nullptr;
    ui_pass.occlusionQuerySet = nullptr;
    ui_pass.depthStencilAttachment = nullptr;

    WGPURenderPassColorAttachment ui_color_attach{};
    ui_color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
    ui_color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    ui_color_attach.loadOp = WGPULoadOp_Load;
    ui_color_attach.storeOp = WGPUStoreOp_Store;
    ui_color_attach.view = surface_view;

    ui_pass.colorAttachmentCount = 1;
    ui_pass.colorAttachments = &ui_color_attach;

    WGPURenderPassDepthStencilAttachment ui_depth_attach{};
    ui_depth_attach.depthClearValue = 1.0;
    ui_depth_attach.depthLoadOp = WGPULoadOp_Clear;
    ui_depth_attach.depthStoreOp = WGPUStoreOp_Store;
    ui_depth_attach.view = m_depth_texture->handle_view();
    ui_pass.depthStencilAttachment = &ui_depth_attach;

    WGPURenderPassEncoder ui_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &ui_pass);
    for (Ref<Entity> entity : world->get_dimension(0).get_entities())
        entity->draw_ui(ui_encoder);

    wgpuRenderPassEncoderEnd(ui_encoder);
    wgpuRenderPassEncoderRelease(ui_encoder);

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

const glm::vec3 daylight_color(1.0, 1.0, 1.0);
const glm::vec3 sunset_color(247.0 / 255.0, 152.0 / 255.0, 2.0 / 255.0);

glm::vec3 lerp(const glm::vec3& x, const glm::vec3& y, float t)
{
    return x * (1.0f - t) + y * t;
}

// TODO:
// To improve sky color gradient, sunset/sunrise should be very short, and night/day should be the same length.
template <const size_t _S>
glm::vec3 color_gradient(const Array<glm::vec3, _S>& colors, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.0f)
        return colors[0];
    if (t >= 1.0f)
        return colors[6];

    const int segments = 6;
    const float segLen = 1.0f / float(segments);
    int si = std::min(int(std::floor(t / segLen)), segments - 1);
    float localT = (t - float(si) * segLen) / segLen;
    return lerp(colors[si], colors[si + 1], localT);
}

void Renderer::record_world(const Ref<World>& world, WGPURenderPassEncoder encoder)
{
    ZoneScoped;

    const Dimension& dim = world->get_dimension(0);
    const Ref<Camera> camera = world->get_active_camera();

    if (camera.is_null())
    {
        return;
    }

    float time_uniform = float(Engine::get().time_of_day()) / float(ticks_per_day);
    set_time(time_uniform);

    const Array<glm::vec3, 7> sky_color_array{daylight_color, sunset_color, daylight_color, daylight_color, daylight_color, sunset_color, daylight_color};

    m_environment.view_matrix = camera->get_view_proj_matrix();
    m_environment.sun_direction = glm::normalize(glm::vec3(glm::rotate(glm::mat4(1.0), float(M_PI) * time_uniform, glm::vec3(0, 0, -1.0)) * glm::vec4(1, 0, 0, 1.0)));
    m_environment.sun_color = glm::vec4(color_gradient(sky_color_array, time_uniform), time_uniform);
    m_env_buffer->update(View(m_environment).as_bytes());

    {
        ZoneScopedN("draw entities");

        for (Ref<Entity> entity : dim.get_entities())
            if (!entity.is_null())
                entity->draw(encoder);
    }

    {
        ZoneScopedN("draw opaque");

        WGPURenderPipeline pipeline = get_pipeline(m_chunk_material);
        wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
        wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_chunk_material->get_bind_group(), 0, nullptr);

        for (const auto& [key, chunk] : dim.get_chunks())
        {
            ChunkPos pos = chunk->pos();
            AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2))
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
                AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2))
                                .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

                if (!camera->frustum().contains(aabb))
                    continue;

                const Ref<Mesh>& mesh = slice.mesh;
                wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 3, chunk->get_chunk_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));

                wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
            }
        }
    }

    {
        ZoneScopedN("draw transparent");

        WGPURenderPipeline pipeline = get_pipeline(m_water_material);
        wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
        wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_water_material->get_bind_group(), 0, nullptr);

        // TODO: sort back to front.

        for (const auto& [key, chunk] : dim.get_chunks())
        {
            ChunkPos pos = chunk->pos();
            AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2))
                            .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            if (!camera->frustum().contains(aabb))
                continue;

            const Chunk::Slice *slices = chunk->get_slices();

            // TODO: do the frustum check per slice.
            for (size_t i = 0; i < Chunk::slice_count; i++)
            {
                const Chunk::Slice& slice = slices[i];

                if (slice.water_mesh.is_null())
                    continue;

                ChunkPos pos = chunk->pos();
                AABB aabb = AABB(-glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height, Chunk::width / 2))
                                .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

                if (!camera->frustum().contains(aabb))
                    continue;

                const Ref<Mesh>& mesh = slice.water_mesh;
                wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
                wgpuRenderPassEncoderSetVertexBuffer(encoder, 3, chunk->get_chunk_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));

                wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
            }
        }
    }

    // {
    //     ZoneScopedN("draw ui");
    //     for (Ref<Entity> entity : dim.get_entities())
    //         entity->draw_ui(encoder);
    // }
}

void Renderer::set_time(float time)
{
    m_sky_uniforms.time = time;
    m_sky_uniform_buffer->update(View(m_sky_uniforms).as_bytes());
}

WGPURenderPipeline Renderer::get_pipeline(Ref<Material> material, Option<WGPUTextureFormat> color_format, WGPUTextureFormat depth_format)
{
    return EXPECT(m_pipeline_cache.get({
        .shader = material->get_shader(),
        .bind_group_layout = material->get_shader()->get_bind_group_layout(),
        .uv_type = material->get_uv_type(),
        .flags = material->flags(),
        .cull_mode = material->get_cull_mode(),
        .attributes = material->get_attributes(),
        .instance_stride = material->get_instance_stride(),
        .color_format = color_format.value_or(m_color_format),
        .depth_format = depth_format,
        .has_color_attach = !color_format.has_value() || color_format != WGPUTextureFormat_Undefined,
        .has_depth_attach = depth_format != WGPUTextureFormat_Undefined,
    }));
}

WGPUComputePipeline Renderer::get_compute_pipeline(Ref<Shader> shader)
{
    return EXPECT(m_pipeline_cache.get_compute(PipelineCache::ComputeKey(shader)));
}

void Renderer::record_simple_shape(WGPURenderPassEncoder encoder, Ref<Material> material, WGPUTextureFormat color_format)
{
    if (color_format == WGPUTextureFormat_Undefined)
        color_format = m_color_format;

    const Ref<Mesh>& mesh = m_cube_mesh;

    WGPURenderPipeline pipeline = get_pipeline(material, color_format);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, material->get_bind_group(), 0, nullptr);

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

    wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
}

View<uint8_t> Renderer::get_missing_texture_data() const
{
    return View(missing_texture_data, 16 * 16).as_bytes();
}

