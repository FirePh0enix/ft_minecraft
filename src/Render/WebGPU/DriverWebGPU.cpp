#include "Render/WebGPU/DriverWebGPU.hpp"
#include "Core/Containers/StackVector.hpp"
#include "Render/Graph.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#ifdef __platform_web
#define WGPU_STRING_VIEW_INIT nullptr
#define WGPU_STRING_VIEW(NAME) (NAME)
#define WGPUOptionalBool_True true
#else
#define WGPU_STRING_VIEW(NAME) {NAME, WGPU_STRLEN}
#endif

template <typename T>
static bool find_chain_entry(WGPUSType type, WGPUChainedStructOut *chain, T& out)
{
    while (chain && chain->sType != type)
        chain = chain->next;

    if (chain != nullptr)
    {
        out = *(T *)chain;
        return true;
    }
    else
    {
        return false;
    }
}

static WGPUTextureFormat convert_texture_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8Unorm:
        return WGPUTextureFormat_R8Unorm;
    case TextureFormat::RGBA8Unorm:
        return WGPUTextureFormat_RGBA8Unorm;
    case TextureFormat::RGBA8Srgb:
        return WGPUTextureFormat_RGBA8UnormSrgb;
    case TextureFormat::BGRA8Srgb:
        return WGPUTextureFormat_BGRA8UnormSrgb;
    case TextureFormat::R32Sfloat:
        return WGPUTextureFormat_R32Float;
    case TextureFormat::RG32Sfloat:
        return WGPUTextureFormat_RG32Float;
    case TextureFormat::RGBA32Sfloat:
        return WGPUTextureFormat_RGBA32Float;
    case TextureFormat::Depth32:
        return WGPUTextureFormat_Depth32Float;
    default:
        // RGB32Sfloat not supported
        std::abort();
        break;
    }
    return {};
}

static WGPUVertexFormat convert_shader_type(ShaderType type)
{
    switch (type)
    {
    case ShaderType::Float32:
        return WGPUVertexFormat_Float32;
    case ShaderType::Float32x2:
        return WGPUVertexFormat_Float32x2;
    case ShaderType::Float32x3:
        return WGPUVertexFormat_Float32x3;
    case ShaderType::Float32x4:
        return WGPUVertexFormat_Float32x4;
    case ShaderType::Uint32:
        return WGPUVertexFormat_Uint32;
    case ShaderType::Uint32x2:
        return WGPUVertexFormat_Uint32x2;
    case ShaderType::Uint32x3:
        return WGPUVertexFormat_Uint32x3;
    case ShaderType::Uint32x4:
        return WGPUVertexFormat_Uint32x4;
    }
    return WGPUVertexFormat_Force32;
}

static WGPUBufferUsage convert_buffer_usage(BufferUsageFlags usage)
{
    uint32_t flags = {};

    if (usage.has_any(BufferUsageFlagBits::CopySource))
        flags |= WGPUBufferUsage_CopySrc;
    if (usage.has_any(BufferUsageFlagBits::CopyDest))
        flags |= WGPUBufferUsage_CopyDst;
    if (usage.has_any(BufferUsageFlagBits::Uniform))
        flags |= WGPUBufferUsage_Uniform;
    if (usage.has_any(BufferUsageFlagBits::Index))
        flags |= WGPUBufferUsage_Index;
    if (usage.has_any(BufferUsageFlagBits::Vertex))
        flags |= WGPUBufferUsage_Vertex;
    if (usage.has_any(BufferUsageFlagBits::Storage))
        flags |= WGPUBufferUsage_Storage;

    return (WGPUBufferUsage)flags;
}

static WGPUTextureUsage convert_texture_usage(TextureUsageFlags usage)
{
    uint32_t flags = {};

    if (usage.has_any(TextureUsageFlagBits::ColorAttachment | TextureUsageFlagBits::DepthAttachment))
        flags |= WGPUTextureUsage_RenderAttachment;
    if (usage.has_any(TextureUsageFlagBits::CopySource))
        flags |= WGPUTextureUsage_CopySrc;
    if (usage.has_any(TextureUsageFlagBits::CopyDest))
        flags |= WGPUTextureUsage_CopyDst;
    if (usage.has_any(TextureUsageFlagBits::Sampled))
        flags |= WGPUTextureUsage_TextureBinding;

    return (WGPUTextureUsage)flags;
}

static WGPUIndexFormat convert_index_type(IndexType type)
{
    switch (type)
    {
    case IndexType::Uint16:
        return WGPUIndexFormat_Uint16;
    case IndexType::Uint32:
        return WGPUIndexFormat_Uint32;
    }
    return {};
}

static WGPUShaderStage convert_shader_stage(ShaderStageFlagBits kind)
{
    switch (kind)
    {
    case ShaderStageFlagBits::Vertex:
        return WGPUShaderStage_Vertex;
    case ShaderStageFlagBits::Fragment:
        return WGPUShaderStage_Fragment;
    case ShaderStageFlagBits::Compute:
        return WGPUShaderStage_Compute;
    }
    return {};
}

static WGPUCullMode convert_cull_mode(CullMode cull_mode)
{
    switch (cull_mode)
    {
    case CullMode::None:
        return WGPUCullMode_None;
    case CullMode::Front:
        return WGPUCullMode_Front;
    case CullMode::Back:
        return WGPUCullMode_Back;
    };
    return {};
}

static WGPUAddressMode convert_address_mode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::ClampToEdge:
        return WGPUAddressMode_ClampToEdge;
    case AddressMode::Repeat:
        return WGPUAddressMode_Repeat;
    }
    return {};
}

static WGPUFilterMode convert_filter_mode(Filter filter)
{
    switch (filter)
    {
    case Filter::Linear:
        return WGPUFilterMode_Linear;
    case Filter::Nearest:
        return WGPUFilterMode_Nearest;
    }
    return {};
}

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

static WGPUSamplerDescriptor convert_sampler(SamplerDescriptor sampler)
{
    WGPUSamplerDescriptor desc{};
    desc.magFilter = convert_filter_mode(sampler.mag_filter);
    desc.minFilter = convert_filter_mode(sampler.min_filter);
    desc.addressModeU = convert_address_mode(sampler.address_mode.u);
    desc.addressModeV = convert_address_mode(sampler.address_mode.v);
    desc.addressModeW = convert_address_mode(sampler.address_mode.w);

    desc.maxAnisotropy = 1;

    return desc;
}

static std::vector<WGPUBindGroupLayoutEntry> convert_bindings(const Ref<Shader>& shader)
{
    std::vector<WGPUBindGroupLayoutEntry> entries;
    entries.reserve(shader->get_bindings().size());

    for (const auto& binding_pair : shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = convert_shader_stage(binding.shader_stage);
            entry.texture.sampleType = WGPUTextureSampleType_Float;
            entry.texture.multisampled = false;
            entry.texture.viewDimension = convert_texture_view_dimension(binding.dimension);

            entries.push_back(entry);

            WGPUBindGroupLayoutEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.visibility = convert_shader_stage(binding.shader_stage);
            sampler_entry.sampler.nextInChain = nullptr;
            sampler_entry.sampler.type = WGPUSamplerBindingType_Filtering;

            entries.push_back(sampler_entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = convert_shader_stage(binding.shader_stage);
            entry.buffer.type = WGPUBufferBindingType_Uniform;

            entries.push_back(entry);
        }
        break;
        case BindingKind::StorageBuffer:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = convert_shader_stage(binding.shader_stage);
            // Vertex shaders does not support Writable storage buffer, a feature can be enabled in wgpu-native but it
            // only works on native.
            entry.buffer.type = binding.shader_stage.has_any(ShaderStageFlagBits::Vertex) ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;

            entries.push_back(entry);
        }
        break;
        default:
            ASSERT(false, "NOT IMPLEMENTED");
            break;
        }
    }

    return entries;
}

MaterialLayoutCacheValue MaterialLayoutCache::create_object(const MaterialLayoutCacheKey& key)
{
    const Ref<Shader>& shader = key.material->get_shader();
    std::vector<WGPUBindGroupLayoutEntry> entries = convert_bindings(shader);

    WGPUBindGroupLayoutDescriptor bind_group_desc{};
    bind_group_desc.entryCount = entries.size();
    bind_group_desc.entries = entries.data();

    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(RenderingDriverWebGPU::get()->get_device(), &bind_group_desc);
    ERR_COND_R(bind_group_layout == nullptr, "BindGroupLayout is invalid", {});

    // std::array<WGPUBindGroupLayout, 2> layouts{bind_group_layout, m_push_constant_layout};
    std::array<WGPUBindGroupLayout, 1> layouts{bind_group_layout};

    WGPUPipelineLayoutDescriptor pipeline_layout_desc{};
    pipeline_layout_desc.bindGroupLayoutCount = layouts.size();
    pipeline_layout_desc.bindGroupLayouts = layouts.data();

#ifndef __platform_web
    std::vector<WGPUPushConstantRange> ranges;
    ranges.reserve(shader->get_push_constants().size());

    // TODO: I didn't manage to detect which shader stages uses a push constant so for now let's hardcode it for now.
    //       WebGPU want an exact match (for perfomance reason ?).
    WGPUShaderStage stages = key.material->is<Material>() ? WGPUShaderStage_Vertex : WGPUShaderStage_Compute;

    for (const PushConstantRange& range : shader->get_push_constants())
        ranges.push_back(WGPUPushConstantRange(stages, 0, range.size));

    WGPUPipelineLayoutExtras extras{};
    extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_PipelineLayoutExtras};
    extras.pushConstantRangeCount = ranges.size();
    extras.pushConstantRanges = ranges.data();
    pipeline_layout_desc.nextInChain = &extras.chain;
#endif

    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(RenderingDriverWebGPU::get()->get_device(), &pipeline_layout_desc);
    ERR_COND_R(pipeline_layout == nullptr, "PipelineLayout is invalid", {});

    return MaterialLayoutCacheValue{.bind_group_layout = bind_group_layout, .pipeline_layout = pipeline_layout};
}

WGPURenderPipeline RenderPipelineCache::create_object(const RenderPipelineCacheKey& key)
{
    const Ref<Material>& material = key.material;
    const MaterialLayoutCacheValue& cached_layout = RenderingDriverWebGPU::get()->get_material_layout_cache().get({.material = material});

    auto pipeline_result = RenderingDriverWebGPU::get()->create_render_pipeline(material->get_shader(), material->get_instance_layout(), convert_cull_mode(material->get_cull_mode()), material->get_flags(), cached_layout.pipeline_layout, key.color_attachs, key.previous_depth_pass);
    return pipeline_result.value();
}

// TODO: Compute pipeline should be reused between materials.
WGPUComputePipeline ComputePipelineCache::create_object(const ComputePipelineCacheKey& key)
{
    const MaterialLayoutCacheValue& cached_layout = RenderingDriverWebGPU::get()->get_material_layout_cache().get({.material = key.material});

    auto pipeline_result = RenderingDriverWebGPU::get()->create_compute_pipeline(key.material->get_shader(), cached_layout.pipeline_layout);
    return pipeline_result.value();
}

WGPUSampler SamplerCache::create_object(const SamplerDescriptor& sampler)
{
    WGPUSamplerDescriptor desc = convert_sampler(sampler);
    return wgpuDeviceCreateSampler(RenderingDriverWebGPU::get()->get_device(), &desc);
}

WGPUBindGroup BindGroupCache::get(Ref<MaterialBase> material)
{
    auto iter = m_bind_groups.find(BindGroupCacheKey{material});

    if (iter == m_bind_groups.end() || material->has_param_changed())
    {
        // Create or recreate the bind group.
        if (iter != m_bind_groups.end())
            wgpuBindGroupRelease(iter->second);

        std::vector<WGPUBindGroupEntry> entries;
        entries.reserve(material->get_shader()->get_bindings().size());

        for (const auto& [name, binding] : material->get_shader()->get_bindings())
        {
            switch (binding.kind)
            {
            case BindingKind::Texture:
            {
                const MaterialParamCache& cache = material->get_param(name);

                WGPUBindGroupEntry entry{};
                entry.binding = binding.binding;
                entry.textureView = ((TextureWebGPU *)cache.texture.texture)->view;

                entries.push_back(entry);

                SamplerDescriptor sampler = material->get_shader()->get_sampler(name);

                WGPUSampler sampler_result = RenderingDriverWebGPU::get()->get_sampler_cache().get(sampler);
                ERR_COND_B(sampler_result == nullptr, "Unable to create a sampler");

                WGPUBindGroupEntry sampler_entry{};
                sampler_entry.binding = binding.binding + 1;
                sampler_entry.sampler = sampler_result;

                entries.push_back(sampler_entry);
            }
            break;
            case BindingKind::UniformBuffer:
            {
                const MaterialParamCache& cache = material->get_param(name);
                ASSERT(cache.buffer.buffer->flags().has_any(BufferUsageFlagBits::Uniform), "Missing Uniform flag on buffer");

                WGPUBindGroupEntry entry{};
                entry.binding = binding.binding;
                entry.buffer = ((BufferWebGPU *)cache.buffer.buffer)->buffer;
                entry.offset = 0;
                entry.size = cache.buffer.buffer->size_bytes();

                entries.push_back(entry);
            }
            break;
            case BindingKind::StorageBuffer:
            {
                const MaterialParamCache& cache = material->get_param(name);
                ASSERT(cache.buffer.buffer->flags().has_any(BufferUsageFlagBits::Storage), "Missing Storage flag on buffer");

                WGPUBindGroupEntry entry{};
                entry.binding = binding.binding;
                entry.buffer = ((BufferWebGPU *)cache.buffer.buffer)->buffer;
                entry.offset = 0;
                entry.size = cache.buffer.buffer->size_bytes();

                entries.push_back(entry);
            }
            break;
            }
        }

        const MaterialLayoutCacheValue& value = RenderingDriverWebGPU::get()->get_material_layout_cache().get(MaterialLayoutCacheKey{.material = material});

        WGPUBindGroupDescriptor desc{};
        desc.nextInChain = nullptr;
        desc.layout = value.bind_group_layout;
        desc.entries = entries.data();
        desc.entryCount = entries.size();

        WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(RenderingDriverWebGPU::get()->get_device(), &desc);
        ERR_COND_R(bind_group == nullptr, "Invalid bind group", nullptr);
        m_bind_groups[BindGroupCacheKey{material}] = bind_group;
        material->clear_param_changed();

        return bind_group;
    }

    return iter->second;
}

RenderingDriverWebGPU::RenderingDriverWebGPU()
{
}

RenderingDriverWebGPU::~RenderingDriverWebGPU()
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

#endif

Result<> RenderingDriverWebGPU::initialize(const Window& window, bool enable_validation)
{
    (void)enable_validation;

#ifndef __platform_web
    WGPUInstanceDescriptor instance_desc{};

    WGPUInstanceExtras extras{};
    extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_InstanceExtras};
    extras.backends = WGPUInstanceBackend_Primary;

    if (enable_validation)
        extras.flags |= WGPUInstanceFlag_Validation;

    instance_desc.nextInChain = &extras.chain;

    m_instance = wgpuCreateInstance(&instance_desc);
#else
    m_instance = wgpuCreateInstance(nullptr);
#endif

    m_surface = create_surface(m_instance, window.get_window_ptr());

#ifdef __platform_web
    // On the web we use glue code to acquire a WGPUDevice.
    m_device = emscripten_webgpu_get_device();
    if (!m_device)
        return Error(ErrorKind::BadDriver);
#else
    m_adapter = request_adapter_sync(m_instance);

    // TODO: Use this maybe ?
    // wgpuInstanceEnumerateAdapters(WGPUInstance instance, const WGPUInstanceEnumerateAdapterOptions *options, WGPUAdapter *adapters);

    const WGPUFeatureName required_features[] = {
        (WGPUFeatureName)WGPUNativeFeature_PushConstants,
    };

    WGPUDeviceDescriptor device_desc{};
    device_desc.nextInChain = nullptr;
    device_desc.requiredFeatures = required_features;
    device_desc.requiredFeatureCount = sizeof(required_features) / sizeof(WGPUFeatureName);
    device_desc.requiredLimits = nullptr;
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
            std::string s;
            s.append(message.data, message.length);
            println("{}", s);
        },
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };

    WGPUNativeLimits native_limits{};
    native_limits.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_NativeLimits};
    native_limits.maxPushConstantSize = 128; // Vulkan by spec requires minimum of 128 bytes

    WGPULimits limits{};
    wgpuAdapterGetLimits(m_adapter, &limits);
    limits.nextInChain = &native_limits.chain;

    device_desc.requiredLimits = &limits;

    m_device = request_device_sync(m_adapter, device_desc);
#endif

    m_queue = wgpuDeviceGetQueue(m_device);
    if (!m_queue)
        return Error(ErrorKind::BadDriver);

    // Create a bind group for push constants since WebGPU does not support them.
    // WGPUBindGroupLayoutEntry bind_group_entry{};
    // bind_group_entry.binding = 0;
    // bind_group_entry.buffer.type = WGPUBufferBindingType_Uniform;
    // bind_group_entry.visibility = WGPUShaderStage_Vertex;

    // WGPUBindGroupLayoutDescriptor push_constant_desc{};
    // push_constant_desc.entryCount = 1;
    // push_constant_desc.entries = &bind_group_entry;

    // m_push_constant_layout = m_device.CreateBindGroupLayout(&push_constant_desc);
    // m_push_constant_layout = wgpuDeviceCreateBindGroupLayout(m_device, &push_constant_desc);
    // if (!m_push_constant_layout)
    //     return Error(ErrorKind::BadDriver);

    YEET(configure_surface(window, VSync::On));

    return 0;
}

Result<> RenderingDriverWebGPU::initialize_imgui(const Window& window)
{
#ifdef __has_debug_menu
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
#endif
    return 0;
}

Result<> RenderingDriverWebGPU::configure_surface(const Window& window, VSync vsync)
{
    Extent2D surface_extent = window.size();

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

    auto depth_texture_result = create_texture(surface_extent.width, surface_extent.height, TextureFormat::Depth32, TextureUsageFlagBits::DepthAttachment, TextureDimension::D2D, 1);
    YEET(depth_texture_result);

    Ref<TextureWebGPU> depth_texture = depth_texture_result.value().cast_to<TextureWebGPU>();

    wgpuSurfaceConfigure(m_surface, &config);
    m_surface_extent = surface_extent;
    m_depth_texture = depth_texture;
    m_surface_format = config.format;

    return 0;
}

void RenderingDriverWebGPU::poll()
{
#ifdef __has_debug_menu
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
#endif
}

void RenderingDriverWebGPU::limit_frames(uint32_t limit)
{
    (void)limit;
    // TODO
}

Result<Ref<Buffer>> RenderingDriverWebGPU::create_buffer(const char *name, size_t size, BufferUsageFlags usage, BufferVisibility visibility)
{
    const Struct& element = CoreRegistry::get().get_struct(name);
    const size_t size_bytes = size * element.size;

    WGPUBufferDescriptor desc{};
    desc.size = size_bytes;
    desc.mappedAtCreation = false;
    desc.usage = convert_buffer_usage(usage);

    if (visibility == BufferVisibility::GPUAndCPU)
        desc.usage |= WGPUBufferUsage_MapRead;

    // WebGPU requires the size of an uniform buffer to a multiple of 16 bytes.
    if (usage.has_any(BufferUsageFlagBits::Uniform) && size_bytes % 16 != 0)
        desc.size = (((size_bytes - 1) / 16) + 1) * 16;

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(m_device, &desc);
    ERR_COND_VRV(buffer == nullptr, Error(ErrorKind::OutOfDeviceMemory), "Failed to create buffer of size {}", size_bytes);

    return newobj(BufferWebGPU, buffer, element, size, usage).cast_to<Buffer>();
}

Result<Ref<Texture>> RenderingDriverWebGPU::create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage, TextureDimension dimension, uint32_t layers)
{
    WGPUTextureFormat format_wgpu = convert_texture_format(format);

    WGPUTextureDescriptor desc{};
    desc.usage = convert_texture_usage(usage);
    desc.dimension = convert_texture_dimension(dimension);
    desc.size = WGPUExtent3D(width, height, layers == 0 ? 1 : layers);
    desc.format = format_wgpu;
    desc.viewFormatCount = 1;
    desc.viewFormats = &format_wgpu;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;

    WGPUTexture texture = wgpuDeviceCreateTexture(m_device, &desc);
    if (!texture)
        return Error(ErrorKind::OutOfDeviceMemory);

    WGPUTextureViewDescriptor view_desc{};
    view_desc.format = format_wgpu;
    view_desc.dimension = convert_texture_view_dimension(dimension);
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = layers == 0 ? 1 : layers;

    WGPUTextureView view = wgpuTextureCreateView(texture, &view_desc);
    if (!view)
        return Error(ErrorKind::OutOfDeviceMemory);

    return newobj(TextureWebGPU, texture, view, width, height, format).cast_to<Texture>();
}

void RenderingDriverWebGPU::draw_graph(const RenderGraph& graph)
{
    WGPUSurfaceTexture surface_texture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);

    ERR_COND_VR(surface_texture.texture == nullptr, "Cannot acquire a swapchain image (status = {})", surface_texture.status);

    WGPUTextureView view = wgpuTextureCreateView(surface_texture.texture, nullptr);
    ERR_COND_R(view == nullptr, "Cannot acquire a swapchain image view");

    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);
    WGPUCommandEncoder compute_command_encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);
    WGPURenderPassEncoder render_pass_encoder = nullptr;
    RenderPassDescriptor render_pass_descriptor;
    WGPUComputePassEncoder compute_pass_encoder = nullptr;
    uint32_t render_pass_index = 0;
    bool use_previous_depth_pass = false;

    for (const auto& instruction : graph.get_instructions())
    {
        if (std::holds_alternative<BeginRenderPassInstruction>(instruction))
        {
            const BeginRenderPassInstruction& render_pass = std::get<BeginRenderPassInstruction>(instruction);
            const RenderGraphCache::RenderPass& render_pass_cache = m_render_graph_cache.set_render_pass(render_pass_index, render_pass.descriptor);

            ASSERT(render_pass.descriptor.color_attachments.size() <= 4, "Only supports 4 color attachment");

            WGPURenderPassDescriptor desc{};

            InplaceVector<WGPURenderPassColorAttachment, 4> color_attachs;
            WGPURenderPassDepthStencilAttachment depth_attach{};

            for (const auto& color_attach : render_pass_cache.attachments)
            {
                WGPURenderPassColorAttachment attach{};
                attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
                attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
                attach.loadOp = WGPULoadOp_Clear;
                attach.storeOp = WGPUStoreOp_Store;
                attach.view = color_attach.surface_texture ? view : color_attach.texture->view;

                color_attachs.push_back(attach);
            }

            desc.colorAttachmentCount = color_attachs.size();
            desc.colorAttachments = color_attachs.data();

            if (render_pass_cache.depth_attachment)
            {
                depth_attach.depthClearValue = 1.0;
                depth_attach.depthLoadOp = render_pass_cache.descriptor.depth_attachment->load ? WGPULoadOp_Load : WGPULoadOp_Clear;
                depth_attach.depthStoreOp = render_pass_cache.descriptor.depth_attachment->save ? WGPUStoreOp_Store : WGPUStoreOp_Discard;
                depth_attach.view = render_pass_cache.depth_attachment->depth_load_previous ? m_render_graph_cache.get_render_pass((int32_t)render_pass_index - 1).depth_attachment->texture->view : render_pass_cache.depth_attachment->texture->view;

                desc.depthStencilAttachment = &depth_attach;
            }

            render_pass_encoder = wgpuCommandEncoderBeginRenderPass(command_encoder, &desc);
            use_previous_depth_pass = render_pass.descriptor.depth_attachment.has_value() && render_pass.descriptor.depth_attachment->load;
            render_pass_index += 1;
            render_pass_descriptor = render_pass.descriptor;
        }
        else if (std::holds_alternative<EndRenderPassInstruction>(instruction))
        {
            wgpuRenderPassEncoderEnd(render_pass_encoder);
            wgpuRenderPassEncoderRelease(render_pass_encoder);
            render_pass_encoder = nullptr;
        }
        else if (std::holds_alternative<DrawInstruction>(instruction))
        {
            const DrawInstruction& draw = std::get<DrawInstruction>(instruction);
            wgpuRenderPassEncoderDrawIndexed(render_pass_encoder, draw.vertex_count, draw.instance_count, 0, 0, 0);
        }
        else if (std::holds_alternative<BindIndexBufferInstruction>(instruction))
        {
            const BindIndexBufferInstruction& bind = std::get<BindIndexBufferInstruction>(instruction);
            const Ref<BufferWebGPU>& buffer = bind.buffer.cast_to<BufferWebGPU>();
            wgpuRenderPassEncoderSetIndexBuffer(render_pass_encoder, buffer->buffer, convert_index_type(bind.index_type), 0, buffer->size_bytes());
        }
        else if (std::holds_alternative<BindVertexBufferInstruction>(instruction))
        {
            const BindVertexBufferInstruction& bind = std::get<BindVertexBufferInstruction>(instruction);
            const Ref<BufferWebGPU>& buffer = bind.buffer.cast_to<BufferWebGPU>();
            wgpuRenderPassEncoderSetVertexBuffer(render_pass_encoder, bind.location, buffer->buffer, 0, buffer->size_bytes());
        }
        else if (std::holds_alternative<BindMaterialInstruction>(instruction))
        {
            const BindMaterialInstruction& bind = std::get<BindMaterialInstruction>(instruction);

            if (Ref<Material> material = bind.material.cast_to<Material>())
            {
                WGPURenderPipeline pipeline = m_pipeline_cache.get(RenderPipelineCacheKey(bind.material, render_pass_descriptor.color_attachments, use_previous_depth_pass));
                wgpuRenderPassEncoderSetPipeline(render_pass_encoder, pipeline);

                WGPUBindGroup bind_group = m_bind_group_cache.get(bind.material);
                wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, bind_group, 0, nullptr);
            }
            else if (Ref<ComputeMaterial> material = bind.material.cast_to<ComputeMaterial>())
            {
                WGPUComputePipeline pipeline = m_compute_pipeline_cache.get(ComputePipelineCacheKey(bind.material));
                wgpuComputePassEncoderSetPipeline(compute_pass_encoder, pipeline);

                WGPUBindGroup bind_group = m_bind_group_cache.get(bind.material);
                wgpuComputePassEncoderSetBindGroup(compute_pass_encoder, 0, bind_group, 0, nullptr);
            }
        }
        else if (std::holds_alternative<PushConstantsInstruction>(instruction))
        {
            const PushConstantsInstruction& push_constants = std::get<PushConstantsInstruction>(instruction);

#ifndef __platform_web
            if (render_pass_encoder)
                wgpuRenderPassEncoderSetPushConstants(render_pass_encoder, WGPUShaderStage_Vertex, 0, push_constants.buffer.size(), push_constants.buffer.data());
            else if (compute_pass_encoder)
                wgpuComputePassEncoderSetPushConstants(compute_pass_encoder, 0, push_constants.buffer.size(), push_constants.buffer.data());
#else
            ASSERT(false, "unimplemented");
#endif
        }
        else if (std::holds_alternative<BeginComputePassInstruction>(instruction))
        {
            // const BeginComputePassInstruction& pass = std::get<BeginComputePassInstruction>(instruction);

            WGPUComputePassDescriptor desc{};
            desc.label = WGPU_STRING_VIEW_INIT;
            desc.nextInChain = nullptr;
            desc.timestampWrites = nullptr;
            compute_pass_encoder = wgpuCommandEncoderBeginComputePass(compute_command_encoder, &desc);
            // compute_pass_encoder = wgpuCommandEncoderBeginComputePass(command_encoder, &desc);
        }
        else if (std::holds_alternative<EndComputePassInstruction>(instruction))
        {
            wgpuComputePassEncoderEnd(compute_pass_encoder);
            wgpuComputePassEncoderRelease(compute_pass_encoder);
            compute_pass_encoder = nullptr;
        }
        else if (std::holds_alternative<DispatchInstruction>(instruction))
        {
            const DispatchInstruction& dispatch = std::get<DispatchInstruction>(instruction);
            wgpuComputePassEncoderDispatchWorkgroups(compute_pass_encoder, dispatch.group_x, dispatch.group_y, dispatch.group_z);
        }
        else if (std::holds_alternative<CopyInstruction>(instruction))
        {
            const CopyInstruction& copy = std::get<CopyInstruction>(instruction);
            wgpuCommandEncoderCopyBufferToBuffer(command_encoder, copy.src.cast_to<BufferWebGPU>()->buffer, copy.src_offset, copy.dst.cast_to<BufferWebGPU>()->buffer, copy.dst_offset, copy.size);
        }
        else if (std::holds_alternative<ImGuiDrawInstruction>(instruction))
        {
#ifdef __has_debug_menu
            ImGui::Render();
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), render_pass_encoder);
#endif
        }
    }

    WGPUCommandBufferDescriptor compute_buffer_desc{};
    compute_buffer_desc.label = WGPU_STRING_VIEW("Compute CommandBuffer");
    WGPUCommandBuffer compute_command_buffer = wgpuCommandEncoderFinish(compute_command_encoder, &compute_buffer_desc);
    wgpuQueueSubmit(m_queue, 1, &compute_command_buffer);

    WGPUCommandBufferDescriptor buffer_desc{};
    buffer_desc.label = WGPU_STRING_VIEW("Render CommandBuffer");
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, &buffer_desc);
    wgpuQueueSubmit(m_queue, 1, &command_buffer);

    // WGPUCommandBuffer command_buffers[] = {compute_command_buffer, command_buffer};
    // wgpuQueueSubmit(m_queue, 2, command_buffers);

#ifdef __platform_web
    emscripten_request_animation_frame([](double, void *)
                                       { return true; }, nullptr);
#else
    wgpuSurfacePresent(m_surface);
#endif

    wgpuCommandBufferRelease(compute_command_buffer);
    wgpuCommandEncoderRelease(compute_command_encoder);

    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuTextureViewRelease(view);
}

WGPUShaderModule RenderingDriverWebGPU::create_shader_module(const Ref<Shader>& shader)
{
    WGPUShaderModuleDescriptor module_desc{};
    module_desc.label = WGPU_STRING_VIEW_INIT;

#ifndef __platform_web
    std::string s;
    WGPUShaderSourceSPIRV spirv_source{};
    WGPUShaderSourceGLSL glsl_source{};

    if (shader->get_shader_kind() == ShaderKind::SPIRV)
    {
        const View<uint32_t> shader_code = shader->get_code_u32();

        spirv_source.chain = {.next = nullptr, .sType = WGPUSType_ShaderSourceSPIRV};
        spirv_source.code = shader_code.data();
        spirv_source.codeSize = shader_code.size();
        module_desc.nextInChain = &spirv_source.chain;
    }
    else if (shader->get_shader_kind() == ShaderKind::GLSL)
    {
        const View<char> shader_code = shader->get_code();

        glsl_source.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_ShaderSourceGLSL};
        glsl_source.stage = WGPUShaderStage_Compute;
        glsl_source.code.data = shader_code.data();
        glsl_source.code.length = shader_code.size();
        glsl_source.defineCount = 0;
        glsl_source.defines = nullptr;
        module_desc.nextInChain = &glsl_source.chain;
    }
#else
    const View<char> shader_code = shader->get_code();

    WGPUShaderModuleWGSLDescriptor wgsl_source{};
    wgsl_source.chain = {.next = nullptr, .sType = WGPUSType_ShaderModuleWGSLDescriptor};
    wgsl_source.code = shader_code.data();
    module_desc.nextInChain = &wgsl_source.chain;
#endif

    return wgpuDeviceCreateShaderModule(m_device, &module_desc);
}

Result<WGPURenderPipeline> RenderingDriverWebGPU::create_render_pipeline(Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, WGPUCullMode cull_mode, MaterialFlags flags, WGPUPipelineLayout pipeline_layout, const std::vector<RenderPassColorAttachment>& color_attachs, bool previous_depth_pass)
{
    std::vector<WGPUVertexBufferLayout> buffers;
    buffers.reserve(instance_layout.has_value() ? 4 : 3);

    WGPUVertexAttribute vertex_attrib{};
    vertex_attrib.format = WGPUVertexFormat_Float32x3;
    vertex_attrib.offset = 0;
    vertex_attrib.shaderLocation = 0;
    buffers.push_back(WGPUVertexBufferLayout{.stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib});

    WGPUVertexAttribute normal_attrib{};
    normal_attrib.format = WGPUVertexFormat_Float32x3;
    normal_attrib.offset = 0;
    normal_attrib.shaderLocation = 1;
    buffers.push_back(WGPUVertexBufferLayout{.stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib});

    WGPUVertexAttribute uv_attrib{};
    uv_attrib.format = WGPUVertexFormat_Float32x2;
    uv_attrib.offset = 0;
    uv_attrib.shaderLocation = 2;
    buffers.push_back(WGPUVertexBufferLayout{.stepMode = WGPUVertexStepMode_Vertex, .arrayStride = sizeof(glm::vec2), .attributeCount = 1, .attributes = &uv_attrib});

    std::vector<WGPUVertexAttribute> instance_attribs;

    if (instance_layout)
    {
        InstanceLayout layout = instance_layout.value();
        instance_attribs.reserve(layout.inputs.size());

        for (size_t i = 0; i < layout.inputs.size(); i++)
        {
            const InstanceLayoutInput& input = layout.inputs[i];
            const WGPUVertexFormat format = convert_shader_type(input.type);
            instance_attribs.push_back(WGPUVertexAttribute{.format = format, .offset = input.offset, .shaderLocation = static_cast<uint32_t>(3 + i)});
        }

        buffers.push_back(WGPUVertexBufferLayout{.stepMode = WGPUVertexStepMode_Instance, .arrayStride = layout.stride, .attributeCount = instance_attribs.size(), .attributes = instance_attribs.data()});
    }

    WGPUShaderModule module = create_shader_module(shader);
    ERR_COND_R(module == nullptr, "Unable to compile shader", Error(ErrorKind::BadDriver));

    const std::string& vertex_ep = shader->get_entry_point(ShaderStageFlagBits::Vertex);

    WGPUVertexState vertex_state{};
    vertex_state.buffers = buffers.data();
    vertex_state.bufferCount = buffers.size();
    vertex_state.entryPoint = WGPU_STRING_VIEW(vertex_ep.c_str());
    vertex_state.module = module;

    WGPUBlendState blend_state{};

    if (!flags.has_any(MaterialFlagBits::Transparency))
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

    for (const auto& color_attach : color_attachs)
    {
        (void)color_attach;
        color_states.push_back(WGPUColorTargetState{.nextInChain = nullptr, .format = m_surface_format, .blend = &blend_state, .writeMask = WGPUColorWriteMask_All});
    }

    const std::string& fragment_ep = shader->get_entry_point(ShaderStageFlagBits::Fragment);

    WGPUFragmentState fragment_state{};
    fragment_state.targets = color_states.data();
    fragment_state.targetCount = color_states.size();
    fragment_state.entryPoint = WGPU_STRING_VIEW(fragment_ep.c_str());
    fragment_state.module = module;

    WGPUDepthStencilState depth_state{};
    depth_state.format = WGPUTextureFormat_Depth32Float;
    depth_state.depthWriteEnabled = WGPUOptionalBool_True;

    if (previous_depth_pass)
        depth_state.depthCompare = WGPUCompareFunction_Equal;
    else if (flags.has_any(MaterialFlagBits::DrawBeforeEverything))
        depth_state.depthCompare = WGPUCompareFunction_LessEqual;
    else
        depth_state.depthCompare = WGPUCompareFunction_Less;

    WGPUPrimitiveState primitive_state{};
    primitive_state.cullMode = cull_mode;
    primitive_state.frontFace = WGPUFrontFace_CCW; // FIXME
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPURenderPipelineDescriptor desc{};
    desc.layout = pipeline_layout;
    desc.primitive = primitive_state;
    desc.vertex = vertex_state;
    desc.fragment = color_attachs.size() == 0 ? nullptr : &fragment_state;
    desc.depthStencil = &depth_state;
    desc.multisample = WGPUMultisampleState(nullptr, 1, 0xFFFFFFFF, false);

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_device, &desc);
    if (!pipeline)
        return Error(ErrorKind::BadDriver);

    return pipeline;
}

Result<WGPUComputePipeline> RenderingDriverWebGPU::create_compute_pipeline(const Ref<Shader>& shader, WGPUPipelineLayout pipeline_layout)
{
    WGPUShaderModule module = create_shader_module(shader);
    ERR_COND_R(module == nullptr, "Unable to compile shader", Error(ErrorKind::BadDriver));

    // const std::string& entry_point = shader->get_entry_point(ShaderStageFlagBits::Compute);

    WGPUProgrammableStageDescriptor compute_desc{};
    compute_desc.module = module;
    // FIXME: For some reason the entrypoint for compute shaders is always `main`.
    compute_desc.entryPoint = WGPU_STRING_VIEW("main"); // WGPU_STRING_VIEW(entry_point.c_str())

    WGPUComputePipelineDescriptor desc{};
    desc.layout = pipeline_layout;
    desc.compute = compute_desc;

    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(m_device, &desc);
    if (!pipeline)
        return Error(ErrorKind::BadDriver);

    return pipeline;
}

void BufferWebGPU::update(View<uint8_t> view, size_t offset)
{
    wgpuQueueWriteBuffer(RenderingDriverWebGPU::get()->get_queue(), buffer, static_cast<uint64_t>(offset), view.data(), view.size());
}

struct BufferWebGPURead
{
    WGPUBuffer buffer;
    BufferReadCallback callback;
    size_t offset;
    size_t size;
};

void BufferWebGPU::read_async(size_t offset, size_t size, BufferReadCallback callback, void *user)
{
    WGPUQueueWorkDoneCallbackInfo callback_info{};
    callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
    callback_info.userdata1 = new BufferWebGPURead(buffer, callback, offset, size);
    callback_info.userdata2 = user;
    callback_info.callback = [](WGPUQueueWorkDoneStatus status, void *user1, void *user2)
    {
        WGPUBufferMapCallbackInfo map_callback_info{};
        map_callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
        map_callback_info.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void *user1, void *user2)
        {
            (void)status;
            (void)message;
            BufferWebGPURead *read = static_cast<BufferWebGPURead *>(user1);
            const void *ptr = wgpuBufferGetConstMappedRange(read->buffer, read->offset, read->size);
            read->callback(ptr, read->size, user2);
            wgpuBufferUnmap(read->buffer);
            delete read;
        };
        map_callback_info.userdata1 = user1;
        map_callback_info.userdata2 = user2;

        BufferWebGPURead *read = (BufferWebGPURead *)user1;
        wgpuBufferMapAsync(read->buffer, WGPUMapMode_Read, read->offset, read->size, map_callback_info);
    };
    wgpuQueueOnSubmittedWorkDone(RenderingDriverWebGPU::get()->get_queue(), callback_info);
}

void TextureWebGPU::update(View<uint8_t> view, uint32_t layer)
{
#ifndef __platform_web
    WGPUTexelCopyTextureInfo copy_info{};
    copy_info.texture = texture;
    copy_info.aspect = WGPUTextureAspect_All;
    copy_info.origin.x = 0;
    copy_info.origin.y = 0;
    copy_info.origin.z = layer;
    copy_info.mipLevel = 0;

    WGPUTexelCopyBufferLayout layout{};
    layout.bytesPerRow = size_of(format) * m_width;
    layout.rowsPerImage = m_height;
    layout.offset = 0;

    WGPUExtent3D write_size(m_width, m_height, 1);

    wgpuQueueWriteTexture(RenderingDriverWebGPU::get()->get_queue(), &copy_info, view.data(), view.size(), &layout, &write_size);
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

    wgpuQueueWriteTexture(RenderingDriverWebGPU::get()->get_queue(), &copy_info, view.data(), view.size(), &layout, &write_size);
#endif
}

RenderGraphCache::RenderPass& RenderGraphCache::set_render_pass(uint32_t index, RenderPassDescriptor desc)
{
    if (index >= m_render_passes.size())
    {
        RenderGraphCache::RenderPass render_pass_cache{};
        render_pass_cache.descriptor = desc;

        for (const auto& attachment : desc.color_attachments)
        {
            RenderGraphCache::Attachment attach{};
            attach.surface_texture = attachment.surface_texture;

            // FIXME: Textures should be recreated when resizing the window.
            Extent2D extent = RenderingDriver::get()->get_surface_extent();
            Result<Ref<Texture>> texture_result = RenderingDriver::get()->create_texture(extent.width, extent.height, attachment.format, TextureUsageFlagBits::ColorAttachment);
            attach.texture = texture_result.value().cast_to<TextureWebGPU>();

            render_pass_cache.attachments.push_back(attach);
        }

        if (desc.depth_attachment.has_value())
        {
            RenderGraphCache::Attachment attach{};
            attach.depth_load_previous = desc.depth_attachment->load;

            if (!attach.depth_load_previous)
            {
                // FIXME: Textures should be recreated when resizing the window.
                Extent2D extent = RenderingDriver::get()->get_surface_extent();
                Result<Ref<Texture>> texture_result = RenderingDriver::get()->create_texture(extent.width, extent.height, TextureFormat::Depth32, TextureUsageFlagBits::DepthAttachment);
                attach.texture = texture_result.value().cast_to<TextureWebGPU>();
            }

            render_pass_cache.depth_attachment = attach;
        }
        else
        {
            render_pass_cache.depth_attachment = std::nullopt;
        }

        m_render_passes.push_back(render_pass_cache);

        return m_render_passes[index];
    }
    else
    {
        return m_render_passes[index];
    }
}

RenderGraphCache::RenderPass& RenderGraphCache::get_render_pass(int32_t index)
{
    if (index < 0)
    {
        return m_render_passes[m_render_passes.size() + index];
    }
    return m_render_passes[index];
}

#ifdef __platform_macos
// #include <Cocoa/Cocoa.h>
// #include <QuartzCore/CAMetalLayer.h>
#elif defined(__platform_windows)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#endif

WGPUSurface RenderingDriverWebGPU::create_surface(WGPUInstance instance, SDL_Window *window)
{
    SDL_PropertiesID id = SDL_GetWindowProperties(window);
    WGPUSurfaceDescriptor surface_descriptor = {};
    WGPUSurface surface = {};
#if defined(__platform_macos)
    {
        m_metal_view = SDL_Metal_CreateView(window);

        WGPUSurfaceSourceMetalLayer surface_src_metal = {};
        surface_src_metal.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        surface_src_metal.layer = SDL_Metal_GetLayer(m_metal_view);
        surface_descriptor.nextInChain = &surface_src_metal.chain;
        surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);
    }
#elif defined(__platform_linux)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
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
    else if (!SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11"))
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
