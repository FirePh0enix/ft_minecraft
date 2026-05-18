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
#include "Render/Graph.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include "webgpu/webgpu.h"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#include <imgui.h>

#include <thread>
#include <webgpu/wgpu.h>

#include <mutex>

#ifdef __platform_web
#define WGPU_STRING_VIEW_INIT nullptr
#define WGPU_STRING_VIEW(NAME) (NAME)
#define WGPUOptionalBool_True true
#else
#define WGPU_STRING_VIEW(NAME) {NAME, WGPU_STRLEN}
#endif

static WGPUTextureViewDimension convert_texture_view_dimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::D1D:
        return WGPUTextureViewDimension_1D;
    case TextureDimension::D2D:
        return WGPUTextureViewDimension_2D;
    case TextureDimension::D2DArray:
        return WGPUTextureViewDimension_2DArray;
    case TextureDimension::D3D:
        return WGPUTextureViewDimension_3D;
    case TextureDimension::Cube:
        return WGPUTextureViewDimension_Cube;
    case TextureDimension::CubeArray:
        return WGPUTextureViewDimension_CubeArray;
    }
    return {};
}

static WGPUTextureDimension convert_texture_dimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::D1D:
        return WGPUTextureDimension_1D;
    case TextureDimension::D2D:
    case TextureDimension::D2DArray:
    case TextureDimension::Cube:
    case TextureDimension::CubeArray:
        return WGPUTextureDimension_2D;
    case TextureDimension::D3D:
        return WGPUTextureDimension_3D;
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

    Ref<Buffer> buffer = TRY(newref<Buffer>());
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
    wgpuTextureViewRelease(m_view);
    wgpuTextureRelease(m_texture);
}

Result<Ref<Texture>> Texture::create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage, TextureDimension dimension, uint32_t layers, uint32_t mip_level)
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
    view_desc.dimension = convert_texture_view_dimension(dimension);
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = mip_level;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = layers == 0 ? 1 : layers;

    WGPUTextureView view = wgpuTextureCreateView(texture, &view_desc);
    if (!view)
        return Error(ErrorKind::OutOfDeviceMemory);

    Ref<Texture> tex = TRY(newref<Texture>());
    tex->m_texture = texture;
    tex->m_view = view;
    tex->m_width = width;
    tex->m_height = height;
    tex->m_layers = layers;
    tex->m_mip_level = mip_level;
    tex->m_format = format;

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

    Ref<Texture> texture = TRY(Texture::create(w, h, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2D, 1, 1));
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
    Ref<Material> material = TRY(newref<Material>());
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

    EXPECT(m_caches.put(name, MaterialParamCache{.kind = BindingKind::Texture, .texture = texture}));
    m_dirty = true;
}

void Material::set_param(const StringView& name, const Ref<Buffer>& buffer)
{
    Option<Binding> binding_result = m_shader->get_binding(name);
    ERR_COND_VR(buffer.is_null(), "Buffer specified for {} is null", name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    EXPECT(m_caches.put(name, MaterialParamCache{.kind = BindingKind::UniformBuffer, .buffer = buffer}));
    m_dirty = true;
}

const MaterialParamCache& Material::get_param(const StringView& name) const
{
    ASSERT_V(m_caches.contains(name), "Cache missing {}", name);
    return *m_caches.get_ptr(name).get();
}

WGPUBindGroup Material::get_bind_group()
{
    if (m_dirty)
        EXPECT(create_bind_group());
    return m_bind_group;
}

Result<void> Material::create_bind_group()
{
    if (m_bind_group != nullptr)
        wgpuBindGroupRelease(m_bind_group);

    LocalVector<WGPUBindGroupEntry> entries;
    // TRY(entries.reserve(m_shader->get_bindings().size()));

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

            TRY(entries.append(entry));

            SamplerDescriptor sampler = m_shader->get_sampler(name);

            WGPUSampler sampler_result = Renderer::get().get_sampler(sampler);
            ERR_COND_B(sampler_result == nullptr, "Unable to create a sampler");

            WGPUBindGroupEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.sampler = sampler_result;

            TRY(entries.append(sampler_entry));
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

            TRY(entries.append(entry));
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

            TRY(entries.append(entry));
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
    ERR_COND_R(m_bind_group == nullptr, "Invalid bind group", Result<void>());

    m_dirty = false;

    return Result<void>();
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
        return pipeline_opt.get();

    LocalVector<WGPUVertexBufferLayout> buffers;
    TRY(buffers.reserve(3 + key.attributes.size()));

    uint32_t attrib_index = 0;

    WGPUVertexAttribute vertex_attrib{};
    vertex_attrib.format = WGPUVertexFormat_Float32x3;
    vertex_attrib.offset = 0;
    vertex_attrib.shaderLocation = attrib_index++;
    TRY(buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib}));

    WGPUVertexAttribute normal_attrib{};
    if (!key.flags.has_any(MaterialFlagBits::NoNormal))
    {
        normal_attrib.format = WGPUVertexFormat_Float32x3;
        normal_attrib.offset = 0;
        normal_attrib.shaderLocation = attrib_index++;
        TRY(buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib}));
    }

    WGPUVertexAttribute uv_attrib{};
    if (!key.flags.has_any(MaterialFlagBits::NoUV))
    {
        if (key.uv_type == UVType::UV)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x2;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            TRY(buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec2), .attributeCount = 1, .attributes = &uv_attrib}));
        }
        else if (key.uv_type == UVType::UVT)
        {
            uv_attrib.format = WGPUVertexFormat_Float32x3;
            uv_attrib.offset = 0;
            uv_attrib.shaderLocation = attrib_index++;
            TRY(buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &uv_attrib}));
        }
    }

    LocalVector<WGPUVertexAttribute> attributes;
    TRY(attributes.reserve(key.attributes.size()));
    for (uint32_t i = 0; i < key.attributes.size(); i++)
    {
        InstanceAttribute attrib = key.attributes.get_unchecked(i);
        TRY(attributes.append(WGPUVertexAttribute{.nextInChain = nullptr, .format = attrib.format, .offset = attrib.offset, .shaderLocation = attrib_index + i}));
    }
    if (key.attributes.size() > 0)
    {
        TRY(buffers.append(WGPUVertexBufferLayout{.nextInChain = nullptr, .stepMode = WGPUVertexStepMode_Instance, .arrayStride = key.instance_stride, .attributeCount = attributes.size(), .attributes = attributes.data()}));
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
    {
        TRY(color_states.append(WGPUColorTargetState{.nextInChain = nullptr, .format = key.color_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All}));
    }

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
    desc.depthStencil = &depth_state;
    desc.multisample = WGPUMultisampleState{.nextInChain = nullptr, .count = 1, .mask = 0xFFFFFFFF, .alphaToCoverageEnabled = false};

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(Renderer::get().device(), &desc);
    if (!pipeline)
        return Error(ErrorKind::BadDriver);

    wgpuShaderModuleRelease(module);

    TRY(m_pipelines.put(key, pipeline));
    return pipeline;
}

void PipelineCache::clear()
{
    for (auto [_, pipeline] : m_pipelines)
        wgpuRenderPipelineRelease(pipeline);
}

WGPUSampler SamplerCache::get(const SamplerDescriptor& desc)
{
    Option<WGPUSampler> sampler_opt = m_samplers.get(desc);
    if (sampler_opt.has_value())
        return sampler_opt.get();

    WGPUSamplerDescriptor d{};
    d.magFilter = desc.mag_filter;
    d.minFilter = desc.min_filter;
    d.addressModeU = desc.address_mode.u;
    d.addressModeV = desc.address_mode.v;
    d.addressModeW = desc.address_mode.w;
    d.maxAnisotropy = 1;

    WGPUSampler sampler = wgpuDeviceCreateSampler(Renderer::get().device(), &d);
    EXPECT(m_samplers.put(desc, sampler));

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
    WGPUFuture future = wgpuInstanceRequestAdapter(instance, nullptr, callback_info);
    (void)future;
    return adapter;
}

WGPUDevice request_device_sync(WGPUAdapter adapter, const WGPUDeviceDescriptor& options)
{
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
        video_driver = "wayland";

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

    WGPUInstanceExtras extras{};
    extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_InstanceExtras};
    extras.backends = WGPUInstanceBackend_Primary;

    if (flags.has_any(InitFlagBits::Validation))
        extras.flags |= WGPUInstanceFlag_Validation;

    instance_desc.nextInChain = &extras.chain;

    m_instance = wgpuCreateInstance(&instance_desc);
#else
    m_instance = wgpuCreateInstance(nullptr);
#endif

    m_surface = create_surface(m_instance, window.get_window_ptr());
    if (m_surface == nullptr)
    {
        error("Unable to create the WGPUSurface");
        return Error(ErrorKind::BadDriver);
    }

#ifdef __platform_web
    // On the web we use glue code to acquire a WGPUDevice.
    m_device = emscripten_webgpu_get_device();
    if (!m_device)
        return Error(ErrorKind::BadDriver);
#else
    m_adapter = request_adapter_sync(m_instance);

    // TODO: Use this maybe ?
    // wgpuInstanceEnumerateAdapters(WGPUInstance instance, const WGPUInstanceEnumerateAdapterOptions *options, WGPUAdapter *adapters);

    const WGPUFeatureName required_features[] = {};

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

    m_device = request_device_sync(m_adapter, device_desc);
    if (!m_device)
    {
        return Error(ErrorKind::NoSuitableDevice);
    }
#endif

    m_queue = wgpuDeviceGetQueue(m_device);
    if (!m_queue)
        return Error(ErrorKind::BadDriver);

    m_env_2d_buffer = TRY(Buffer::create(sizeof(glm::mat4), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    TRY(configure_surface(window.size().width, window.size().height, VSync::On));

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

    m_env_buffer = TRY(Buffer::create(sizeof(WorldEnvironment), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_voxel_shader = TRY(Shader::load("assets/shaders/voxel.wgsl"));
    m_voxel_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, TextureDimension::D2DArray)); // binding = 1 is the sampler
    m_voxel_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_voxel_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_voxel_shader->create_bind_group_layout();

    m_model_shader = TRY(Shader::load("assets/shaders/model.wgsl"));
    m_model_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_model_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_model_shader->set_binding("global_model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_model_shader->set_binding("uvs", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 3, BindingAccess::Read));
    m_model_shader->set_binding("texture", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 4, BindingAccess::Read, TextureDimension::D2D));
    m_model_shader->set_sampler("texture", SamplerDescriptor(WGPUFilterMode_Nearest, WGPUFilterMode_Nearest));
    m_model_shader->create_bind_group_layout();

    m_cube_mesh = TRY(create_cube_mesh());

    m_preview_block_shader = TRY(Shader::load("assets/shaders/block_preview.wgsl"));
    m_preview_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_preview_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 1, BindingAccess::Read, TextureDimension::D2DArray)); // binding = 3 is the sampler
    m_preview_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_preview_block_shader->create_bind_group_layout();

    m_item_block_shader = TRY(Shader::load("assets/shaders/block.wgsl"));
    m_item_block_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_item_block_shader->set_binding("model", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_item_block_shader->set_binding("images", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, TextureDimension::D2DArray)); // binding = 3 is the sampler
    m_item_block_shader->set_sampler("images", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_item_block_shader->create_bind_group_layout();

    EXPECT(Engine::get().registry().register_all());  // TODO: I dont really like having this here but it needs to be before calling `get_texture_array` and after initializing WebGPU.
    EXPECT(Engine::get().registry().post_register()); //       Maybe split this function in two (initializing/creating resources).

    Vector<InstanceAttribute> attributes;
    TRY(attributes.append(InstanceAttribute{.offset = 0, .format = WGPUVertexFormat_Float32x3}));

    m_chunk_material = TRY(Material::create(m_voxel_shader, MaterialFlagBits::None, WGPUCullMode_Back, UVType::UVT, Instance(attributes, sizeof(glm::vec3))));
    m_chunk_material->set_param("images", Engine::get().registry().get_texture_array());
    m_chunk_material->set_param("env", Renderer::get().get_world_environment());

    m_water_shader = TRY(Shader::load("assets/shaders/water.wgsl"));
    m_water_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 0, BindingAccess::Read, TextureDimension::D2D)); // binding = 1 is the sampler
    m_water_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 2, BindingAccess::Read));
    m_water_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_water_shader->create_bind_group_layout();

    m_water_material = TRY(Material::create(m_water_shader, MaterialFlagBits::Transparency, WGPUCullMode_Back, UVType::UV, Instance(attributes, sizeof(glm::vec3))));
    m_water_material->set_param("image", EXPECT(Engine::get().registry().create_texture("assets/textures/water.png")));
    m_water_material->set_param("env", Renderer::get().get_world_environment());

    m_simple_shader = TRY(Shader::load("assets/shaders/simple_shape.wgsl"));
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

    m_color_rect_shader = TRY(Shader::load("assets/shaders/ui/color_rect.wgsl"));
    m_color_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_color_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_color_rect_shader->create_bind_group_layout();

    m_texture_rect_shader = TRY(Shader::load("assets/shaders/ui/tex_rect.wgsl"));
    m_texture_rect_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    m_texture_rect_shader->set_binding("uniforms", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    m_texture_rect_shader->set_binding("image", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, TextureDimension::D2D)); // binding = 3 is the sampler
    m_texture_rect_shader->set_sampler("image", {.min_filter = WGPUFilterMode_Nearest, .mag_filter = WGPUFilterMode_Nearest});
    m_texture_rect_shader->create_bind_group_layout();

    return Result<void>();
}

void Renderer::deinit()
{
    m_sampler_cache.clear();
    m_pipeline_cache.clear();

    wgpuSurfaceRelease(m_surface);
}

Result<void> Renderer::configure_surface(size_t width, size_t height, VSync vsync)
{
    Extent2D surface_extent(width, height);

#ifndef __platform_web
    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);

    WGPUSurfaceConfiguration config{};
    config.device = m_device;
    config.format = capabilities.formats[0];
    config.usage = capabilities.usages;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = vsync == VSync::On ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    config.alphaMode = capabilities.alphaModes[0];
#else
    WGPUSurfaceConfiguration config{};
    config.device = m_device;
    config.format = WGPUTextureFormat_RGBA8UnormSrgb;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = WGPUPresentMode(1); // only FIFO is supported
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
#endif

    wgpuSurfaceConfigure(m_surface, &config);
    m_surface_extent = surface_extent;
    m_surface_format = config.format;

    float ratio = float(width) / float(height);
    glm::mat4 ortho_matrix = glm::ortho(-1.0f * ratio, 1.0f * ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    m_env_2d_buffer->update(View(ortho_matrix).as_bytes());

    return Result<void>();
}

void Renderer::draw(RenderGraph& graph, std::function<void(const RenderPassNode& node)> f)
{
    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", surface_texture.status);
    // TODO: if status is WGPUSurfaceGetCurrentTextureStatus_Outdated or WGPUSurfaceGetCurrentTextureStatus_Timeout

    WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);
    ERR_COND_R(view == nullptr, "Cannot acquire a swapchain image view");

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);
    WGPUCommandBuffer command_buffer = graph.record(encoder, view, [f](const RenderPassNode& node)
                                                    {
                                                        f(node);
                                                        // Draw ImGui only on the main color pass.
                        if (node.is_final_pass())
                        {
                            std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
                            ImGui::Render();
                            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), node.encoder());
                        } });

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

    wgpuTextureViewRelease(view);
}

void Renderer::draw(RenderGraph& graph, Ref<World> world)
{
    draw(graph, [this, world](const RenderPassNode& node)
         { record_world(*this, world, node); });
}

WGPURenderPipeline Renderer::get_pipeline(Ref<Material> material, const RenderPassNode& node)
{
    return EXPECT(m_pipeline_cache.get({
        .shader = material->get_shader(),
        .bind_group_layout = material->get_shader()->get_bind_group_layout(),
        .uv_type = material->get_uv_type(),
        .flags = material->flags(),
        .cull_mode = material->get_cull_mode(),
        .attributes = material->get_attributes(),
        .instance_stride = material->get_instance_stride(),
        .color_format = node.output_to_surface() ? Renderer::get().m_surface_format : (node.get_color_output().is_null() ? WGPUTextureFormat_Undefined : node.get_color_output()->format()),
        .depth_format = WGPUTextureFormat_Depth32Float,
        .has_color_attach = !node.get_color_output().is_null(),
        .has_depth_attach = !node.get_depth_output().is_null(),
    }));
}

void Renderer::record_world(Renderer& renderer, Ref<World> world, const RenderPassNode& node)
{
    Dimension& dim = world->get_dimension(0);
    const Ref<Camera> camera = world->get_active_camera();

    if (camera.is_null())
    {
        return;
    }

    WorldEnvironment env(camera->get_view_proj_matrix());
    renderer.m_env_buffer->update(View(env).as_bytes());

    Ref<Material> material = renderer.m_chunk_material;

    for (Ref<Entity> entity : dim.get_entities())
        if (!entity.is_null())
            entity->draw(node);

    WGPURenderPassEncoder encoder = node.encoder();

    WGPURenderPipeline pipeline = renderer.get_pipeline(material, node);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, material->get_bind_group(), 0, nullptr);

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

            if (slice.mesh.is_null())
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

    pipeline = renderer.get_pipeline(renderer.m_water_material, node);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, renderer.m_water_material->get_bind_group(), 0, nullptr);

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

            const Ref<Mesh>& mesh = slice.water_mesh;
            wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());
            wgpuRenderPassEncoderSetVertexBuffer(encoder, 3, chunk->get_chunk_buffer()->handle(), sizeof(glm::vec3) * i, sizeof(glm::vec3));

            wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
        }
    }

    // TODO: separate UI pass ?
    if (node.is_final_pass())
    {
        for (Ref<Entity> entity : dim.get_entities())
            entity->draw_ui(node);
        Engine::get().encode_debug_menu();
    }
}

void Renderer::record_simple_shape(const RenderPassNode& node, Ref<Material> material)
{
    WGPURenderPassEncoder encoder = node.encoder();
    const Ref<Mesh>& mesh = m_cube_mesh;

    WGPURenderPipeline pipeline = get_pipeline(material, node);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, material->get_bind_group(), 0, nullptr);

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::Normal)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Normal)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 2, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

    wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
}

void Renderer::wait_queue_done()
{
    std::atomic_bool status = true;
    wgpuQueueOnSubmittedWorkDone(m_queue, WGPUQueueWorkDoneCallbackInfo{
                                              .nextInChain = nullptr,
                                              .mode = WGPUCallbackMode_WaitAnyOnly,
                                              .callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void *userdata, void *)
                                              {
                                                  (void)status;
                                                  (void)message;
                                                  *(std::atomic_bool *)userdata = false;
                                              },
                                              .userdata1 = &status,
                                              .userdata2 = nullptr,
                                          });
    while (status)
        std::this_thread::yield();
}
