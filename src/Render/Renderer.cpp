#include "Render/Renderer.hpp"
#include "Core/Error.hpp"
#include "Core/Filesystem.hpp"
#include "Core/String.hpp"
#include "World/World.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <string>

RVRenderer::RVRenderer()
{
}

static WGPUAdapter request_adapter_sync(WGPUInstance instance)
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

static WGPUDevice request_device_sync(WGPUAdapter adapter, const WGPUDeviceDescriptor& options)
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

template <typename T>
static bool has_element(const T *array, size_t array_size, T value)
{
    for (size_t i = 0; i < array_size; i++)
    {
        if (array[i] == value)
            return true;
    }
    return false;
}

void Buffer::clear(WGPUCommandEncoder command_encoder)
{
    wgpuCommandEncoderClearBuffer(command_encoder, buffer, 0, size);
}

void Buffer::copy_to_staging(WGPUCommandEncoder command_encoder)
{
    wgpuCommandEncoderCopyBufferToBuffer(command_encoder, buffer, 0, staging_buffer, 0, size);
}

const void *Buffer::map(WGPUDevice device, uint32_t offset, uint32_t size) const
{
    WGPUBufferMapCallbackInfo info_callback_info{};
    info_callback_info.nextInChain = nullptr;
    info_callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
    info_callback_info.userdata1 = nullptr;
    info_callback_info.userdata2 = nullptr;
    info_callback_info.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void *user1, void *user2)
    {
        (void)status;
        (void)message;
        (void)user1;
        (void)user2;
    };

    const uint32_t map_size = std::min(size, this->size);

    wgpuBufferMapAsync(staging_buffer, WGPUMapMode_Read, 0, map_size, info_callback_info);
    wgpuDevicePoll(device, true, nullptr);

    return wgpuBufferGetConstMappedRange(staging_buffer, offset, map_size);
}

void Buffer::unmap() const
{
    wgpuBufferUnmap(staging_buffer);
}

ComputePipelineBuilder& ComputePipelineBuilder::set_shader(const std::string& path)
{
    Result<ShaderV2> shader_maybe = ShaderV2::load(path);
    const std::string& shader_source = shader_maybe.value().get_source_code();

    m_shader_source = StringView(shader_source);

    return *this;
}

ComputePipeline ComputePipelineBuilder::build()
{
    std::vector<WGPUBindGroupLayout> bind_group_layouts;
    for (const BindGroupLayoutDescriptor& desc : m_bind_group_layout_descriptors)
    {
        const WGPUBindGroupLayoutDescriptor bg_desc{
            .nextInChain = nullptr,
            .label = WGPU_STRING_VIEW_INIT,
            .entryCount = desc.entries.size(),
            .entries = desc.entries.data(),
        };
        bind_group_layouts.push_back(wgpuDeviceCreateBindGroupLayout(m_device, &bg_desc));
    }

    WGPUPipelineLayoutDescriptor pipeline_layout_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .bindGroupLayoutCount = bind_group_layouts.size(),
        .bindGroupLayouts = bind_group_layouts.data(),
    };
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(m_device, &pipeline_layout_desc);

    WGPUShaderSourceWGSL shader_source_desc{
        .chain = {.next = nullptr, .sType = WGPUSType_ShaderSourceWGSL},
        .code = {.data = m_shader_source.data(), .length = m_shader_source.size()},
    };
    WGPUShaderModuleDescriptor shader_desc{
        .nextInChain = &shader_source_desc.chain,
        .label = WGPU_STRING_VIEW_INIT,
    };
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(m_device, &shader_desc);

    WGPUComputePipelineDescriptor pipeline_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .layout = pipeline_layout,
        .compute = WGPUProgrammableStageDescriptor{
            .nextInChain = nullptr,
            .module = shader_module,
            .entryPoint = WGPU_STRING_VIEW("main"),
            .constantCount = 0,
            .constants = nullptr,
        },
    };
    WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(m_device, &pipeline_desc);

    ComputePipeline p{};
    p.pipeline = pipeline;
    p.pipeline_layout = pipeline_layout;
    p.bind_group_layouts = bind_group_layouts;

    return p;
}

Result<> RVRenderer::init(RVInitFlags flags, const Window& window)
{
#ifndef __platform_web
    WGPUInstanceDescriptor instance_desc{};

    WGPUInstanceExtras extras{};
    extras.chain = {.next = nullptr, .sType = (WGPUSType)WGPUSType_InstanceExtras};
    extras.backends = WGPUInstanceBackend_Primary;

    if (flags.has_any(RVInitFlagBits::Validation))
    {
        extras.flags |= WGPUInstanceFlag_Validation;
    }

    instance_desc.nextInChain = &extras.chain;

    m_instance = wgpuCreateInstance(&instance_desc);
#else
    m_instance = wgpuCreateInstance(nullptr);
#endif

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
        WGPUFeatureName_TimestampQuery,
        WGPUFeatureName_BGRA8UnormStorage,

    // 64-bits integers operations are not supported on WebGPU on the web because Metal did not support them 8 years ago...
    // so those operations will be emulated on the web.
#ifndef __platform_web
        (WGPUFeatureName)WGPUNativeFeature_ShaderInt64,
#endif
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

    m_surface = create_surface(m_instance, window.get_window_ptr());

    WGPUSurfaceCapabilities capabilities;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);

    m_surface_format = WGPUTextureFormat_BGRA8UnormSrgb; // capabilities.formats[0];
    m_depth_format = WGPUTextureFormat_Depth32Float;
    m_support_immediate = has_element(capabilities.presentModes, capabilities.presentModeCount, WGPUPresentMode_Immediate);

    WGPUQuerySetDescriptor query_set_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .type = WGPUQueryType_Timestamp,
        .count = 2, // rv_raytracing
    };
    m_query_set = wgpuDeviceCreateQuerySet(m_device, &query_set_desc);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL3_InitForOther(window.get_window_ptr());

    ImGui_ImplWGPU_InitInfo init_info{};
    init_info.Device = m_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = m_surface_format;
    init_info.DepthStencilFormat = m_depth_format;
    ImGui_ImplWGPU_Init(&init_info);

    const WGPUBufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        .size = sizeof(DispatchParams) + 4,
        .mappedAtCreation = false,
    };
    m_rv_raytracing_params_buffer = wgpuDeviceCreateBuffer(m_device, &buffer_desc);

    m_rv_dbg_messages_count = 128;
    m_rv_dbg_messages = create_rw_storage_buffer(sizeof(RVDebugMessage) * m_rv_dbg_messages_count).value();
    m_rv_dbg_messages_info = create_rw_storage_buffer(sizeof(RVDebugMessageInfo)).value();

    return 0;
}

Result<> RVRenderer::configure(const Window& window, RVVSync vsync)
{
    const Extent2D surface_extent = window.size();

    println("configuring surface: {}x{} pixels", surface_extent.width, surface_extent.height);

#ifndef __platform_web
    WGPUSurfaceConfiguration config{};
    config.device = m_device;
    config.format = m_surface_format;
    config.usage = m_surface_usage | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = vsync == RVVSync::On ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    config.alphaMode = m_alpha_mode;
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

    if (m_albedo_texture != nullptr)
    {
        wgpuTextureRelease(m_albedo_texture);
        wgpuTextureViewRelease(m_albedo_texture_view);
    }

    const WGPUTextureFormat albedo_format = WGPUTextureFormat_BGRA8Unorm;
    WGPUTextureDescriptor albedo_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW("(rv) Albedo"),
        .usage = WGPUTextureUsage_StorageBinding | WGPUTextureUsage_CopySrc,
        .dimension = WGPUTextureDimension_2D,
        .size = WGPUExtent3D{.width = m_surface_extent.width, .height = m_surface_extent.height, .depthOrArrayLayers = 1},
        .format = albedo_format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 1,
        .viewFormats = &albedo_format,
    };
    m_albedo_texture = wgpuDeviceCreateTexture(m_device, &albedo_desc);
    m_albedo_texture_view = wgpuTextureCreateView(m_albedo_texture, nullptr);

    if (m_depth_texture != nullptr)
    {
        wgpuTextureRelease(m_depth_texture);
        wgpuTextureViewRelease(m_depth_texture_view);
    }

    WGPUTextureDescriptor depth_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW("(rv) Depth"),
        .usage = WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D,
        .size = WGPUExtent3D{.width = m_surface_extent.width, .height = m_surface_extent.height, .depthOrArrayLayers = 1},
        .format = m_depth_format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 1,
        .viewFormats = &m_depth_format,
    };
    m_depth_texture = wgpuDeviceCreateTexture(m_device, &depth_desc);
    m_depth_texture_view = wgpuTextureCreateView(m_depth_texture, nullptr);

    create_rv_raytracing_resources();

    return 0;
}

Result<Buffer> RVRenderer::create_rw_storage_buffer(uint32_t size)
{
    const WGPUBufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage,
        .size = size,
        .mappedAtCreation = false,
    };
    WGPUBuffer buffer = wgpuDeviceCreateBuffer(m_device, &buffer_desc);
    if (buffer == nullptr)
    {
        return Error(ErrorKind::OutOfDeviceMemory);
    }

    const WGPUBufferDescriptor staging_buffer_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
        .size = size,
        .mappedAtCreation = false,
    };
    WGPUBuffer staging_buffer = wgpuDeviceCreateBuffer(m_device, &staging_buffer_desc);
    if (staging_buffer == nullptr)
    {
        wgpuBufferDestroy(buffer);
        return Error(ErrorKind::OutOfDeviceMemory);
    }

    Buffer b{};
    b.buffer = buffer;
    b.staging_buffer = staging_buffer;
    b.size = size;

    return b;
}

void RVRenderer::prepare_render()
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void RVRenderer::render_world(Ref<World> world)
{
    const Extent2D surface_extent = m_surface_extent;

    prepare_render();
    world->draw(true);

    WGPUSurfaceTexture current_texture;
    wgpuSurfaceGetCurrentTexture(m_surface, &current_texture);

    WGPUTextureView surface_view = wgpuTextureCreateView(current_texture.texture, nullptr);
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

    // m_rv_dbg_messages_info.clear(command_encoder);

    // FIXME: this prevent rendering when the world is not ready which is not the way to do it.
    //        have a small buffer with only one empty node in that case.
    if (world->m_bind_group != nullptr)
    {
        // Update the camera informations.
        // TODO: should be done only when the camera is moving.
        const Ref<Camera>& camera = world->get_active_camera();
        m_rv_raytracing_params.inv_proj_matrix = glm::inverse(camera->get_proj_rot_matrix());
        m_rv_raytracing_params.camera_origin = camera->get_global_transform().position();
        wgpuQueueWriteBuffer(m_queue, m_rv_raytracing_params_buffer, 0, &m_rv_raytracing_params, sizeof(DispatchParams));

        const uint32_t workgroup_x = surface_extent.width / 8;
        const uint32_t workgroup_y = surface_extent.height / 8;
        const uint32_t workgroup_z = 1;

        WGPUComputePassTimestampWrites timestamp_writes{
            .querySet = m_query_set,
            .beginningOfPassWriteIndex = 0,
            .endOfPassWriteIndex = 1,
        };
        WGPUComputePassDescriptor pass_desc{
            .nextInChain = nullptr,
            .label = WGPU_STRING_VIEW_INIT,
            .timestampWrites = &timestamp_writes,
        };

        WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(command_encoder, &pass_desc);
        wgpuComputePassEncoderSetPipeline(pass, m_rv_raytracing_pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, m_rv_raytracing_bind_group, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 1, world->m_bind_group, 0, nullptr);
        wgpuComputePassEncoderSetBindGroup(pass, 2, m_rv_dbg_messages_bind_group, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, workgroup_x, workgroup_y, workgroup_z);
        wgpuComputePassEncoderEnd(pass);
    }

    // Copy from `albedo` to the surface texture after rendering is done.
    const WGPUTexelCopyTextureInfo source{
        .texture = m_albedo_texture,
        .mipLevel = 0,
        .origin = WGPUOrigin3D{.x = 0, .y = 0, .z = 0},
        .aspect = WGPUTextureAspect_Undefined,
    };
    const WGPUTexelCopyTextureInfo dest{
        .texture = current_texture.texture,
        .mipLevel = 0,
        .origin = WGPUOrigin3D{.x = 0, .y = 0, .z = 0},
        .aspect = WGPUTextureAspect_Undefined,
    };
    const WGPUExtent3D copy_size{.width = surface_extent.width, .height = surface_extent.height, .depthOrArrayLayers = 1};
    wgpuCommandEncoderCopyTextureToTexture(command_encoder, &source, &dest, &copy_size);

    // Draw ImGUI on top of everything.
    {
        WGPURenderPassColorAttachment color_attach{
            .nextInChain = nullptr,
            .view = surface_view,
            .depthSlice = 0xFFFFFFFF,
            .resolveTarget = nullptr,
            .loadOp = WGPULoadOp_Load,
            .storeOp = WGPUStoreOp_Store,
            .clearValue = WGPUColor{.r = 0.0, .g = 0.0, .b = 0.0, .a = 0.0},
        };
        WGPURenderPassDepthStencilAttachment depth_attach{
            .view = m_depth_texture_view,
            .depthLoadOp = WGPULoadOp_Clear,
            .depthStoreOp = WGPUStoreOp_Discard,
            .depthClearValue = 1.0,
            .depthReadOnly = false,
            .stencilLoadOp = WGPULoadOp_Undefined,
            .stencilStoreOp = WGPUStoreOp_Undefined,
            .stencilClearValue = 0,
            .stencilReadOnly = false,
        };
        WGPURenderPassDescriptor desc{
            .nextInChain = nullptr,
            .label = WGPU_STRING_VIEW_INIT,
            .colorAttachmentCount = 1,
            .colorAttachments = &color_attach,
            .depthStencilAttachment = &depth_attach,
            .occlusionQuerySet = nullptr,
            .timestampWrites = nullptr,
        };
        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(command_encoder, &desc);

        ImGui::Render();
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

        wgpuRenderPassEncoderEnd(pass);
    }

    // Debug only: copy the back `messages` buffer to the front.
    // {
    //     m_rv_dbg_messages.copy_to_staging(command_encoder);
    //     m_rv_dbg_messages_info.copy_to_staging(command_encoder);
    // }

    // TODO
    // wgpuCommandEncoderResolveQuerySet(command_encoder, m_renderer.m_query_set, 0, 2, WGPUBuffer destination, uint64_t destinationOffset);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, nullptr);
    wgpuQueueSubmit(m_queue, 1, &command_buffer);
    wgpuSurfacePresent(m_surface);

    wgpuTextureViewRelease(surface_view);

    // dump_messages();
}

void RVRenderer::dump_messages()
{
    const RVDebugMessageInfo *info = (RVDebugMessageInfo *)m_rv_dbg_messages_info.map(m_device);
    const uint32_t messages_count = info->count;
    m_rv_dbg_messages_info.unmap();

    const RVDebugMessage *messages = (RVDebugMessage *)m_rv_dbg_messages.map(m_device);
    println("\n>>>");
    println(">>> {} messages:", messages_count);
    println(">>>");
    for (size_t i = 0; i < std::min(messages_count, (uint32_t)m_rv_dbg_messages_count); i++)
    {
        messages[i].dump();
    }
    m_rv_dbg_messages.unmap();
}

void RVRenderer::create_rv_raytracing_resources()
{
    if (m_rv_raytracing_pipeline != nullptr)
    {
        wgpuComputePipelineRelease(m_rv_raytracing_pipeline);
        wgpuPipelineLayoutRelease(m_rv_raytracing_pipeline_layout);

        wgpuBindGroupRelease(m_rv_raytracing_bind_group);
        wgpuBindGroupRelease(m_rv_dbg_messages_bind_group);

        wgpuBindGroupLayoutRelease(m_rv_raytracing_bind_group_layout);
        wgpuBindGroupLayoutRelease(m_rv_raytracing_node_bind_group_layout);
        wgpuBindGroupLayoutRelease(m_rv_dbg_messages_bind_group_layout);
    }

    const WGPUBindGroupLayoutEntry bg_entries[]{
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 0, .visibility = WGPUShaderStage_Compute, .storageTexture = WGPUStorageTextureBindingLayout{.nextInChain = nullptr, .access = WGPUStorageTextureAccess_WriteOnly, .format = WGPUTextureFormat_BGRA8Unorm, .viewDimension = WGPUTextureViewDimension_2D}},
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 1, .visibility = WGPUShaderStage_Compute, .buffer = WGPUBufferBindingLayout{.nextInChain = nullptr, .type = WGPUBufferBindingType_Uniform, .hasDynamicOffset = false, .minBindingSize = 0}},
    };
    WGPUBindGroupLayoutDescriptor bg_layout_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .entryCount = sizeof(bg_entries) / sizeof(WGPUBindGroupLayoutEntry),
        .entries = bg_entries,
    };
    m_rv_raytracing_bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &bg_layout_desc);

    const WGPUBindGroupLayoutEntry bg_node_entries[]{
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 0, .visibility = WGPUShaderStage_Compute, .buffer = WGPUBufferBindingLayout{.nextInChain = nullptr, .type = WGPUBufferBindingType_ReadOnlyStorage, .hasDynamicOffset = false, .minBindingSize = 0}},
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 1, .visibility = WGPUShaderStage_Compute, .buffer = WGPUBufferBindingLayout{.nextInChain = nullptr, .type = WGPUBufferBindingType_ReadOnlyStorage, .hasDynamicOffset = false, .minBindingSize = 0}},
    };
    WGPUBindGroupLayoutDescriptor bg_node_layout_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .entryCount = sizeof(bg_node_entries) / sizeof(WGPUBindGroupLayoutEntry),
        .entries = bg_node_entries,
    };
    m_rv_raytracing_node_bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &bg_node_layout_desc);

    const WGPUBindGroupLayoutEntry bg_messages_entries[]{
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 0, .visibility = WGPUShaderStage_Compute, .buffer = WGPUBufferBindingLayout{.nextInChain = nullptr, .type = WGPUBufferBindingType_Storage, .hasDynamicOffset = false, .minBindingSize = 0}},
        WGPUBindGroupLayoutEntry{.nextInChain = nullptr, .binding = 1, .visibility = WGPUShaderStage_Compute, .buffer = WGPUBufferBindingLayout{.nextInChain = nullptr, .type = WGPUBufferBindingType_Storage, .hasDynamicOffset = false, .minBindingSize = 0}},
    };
    WGPUBindGroupLayoutDescriptor bg_messages_layout_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .entryCount = sizeof(bg_messages_entries) / sizeof(WGPUBindGroupLayoutEntry),
        .entries = bg_messages_entries,
    };
    m_rv_dbg_messages_bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &bg_messages_layout_desc);

    const WGPUBindGroupLayout bind_groups[]{m_rv_raytracing_bind_group_layout, m_rv_raytracing_node_bind_group_layout, m_rv_dbg_messages_bind_group_layout};
    WGPUPipelineLayoutDescriptor pipeline_layout_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .bindGroupLayoutCount = sizeof(bind_groups) / sizeof(WGPUBindGroupLayout),
        .bindGroupLayouts = bind_groups,
    };
    m_rv_raytracing_pipeline_layout = wgpuDeviceCreatePipelineLayout(m_device, &pipeline_layout_desc);

    const WGPUBindGroupEntry entries[]{
        WGPUBindGroupEntry{.nextInChain = nullptr, .binding = 0, .buffer = nullptr, .offset = 0, .size = 0, .sampler = nullptr, .textureView = m_albedo_texture_view},
        WGPUBindGroupEntry{.nextInChain = nullptr, .binding = 1, .buffer = m_rv_raytracing_params_buffer, .offset = 0, .size = sizeof(DispatchParams) + 4, .sampler = nullptr, .textureView = nullptr},
    };
    WGPUBindGroupDescriptor bind_group_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .layout = m_rv_raytracing_bind_group_layout,
        .entryCount = sizeof(entries) / sizeof(WGPUBindGroupEntry),
        .entries = entries,
    };
    m_rv_raytracing_bind_group = wgpuDeviceCreateBindGroup(m_device, &bind_group_desc);

    const WGPUBindGroupEntry dbg_messages_entries[]{
        WGPUBindGroupEntry{.nextInChain = nullptr, .binding = 0, .buffer = m_rv_dbg_messages.buffer, .offset = 0, .size = sizeof(RVDebugMessage) + m_rv_dbg_messages_count, .sampler = nullptr, .textureView = nullptr},
        WGPUBindGroupEntry{.nextInChain = nullptr, .binding = 1, .buffer = m_rv_dbg_messages_info.buffer, .offset = 0, .size = sizeof(RVDebugMessageInfo), .sampler = nullptr, .textureView = nullptr},
    };
    WGPUBindGroupDescriptor dbg_messages_bind_group_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .layout = m_rv_dbg_messages_bind_group_layout,
        .entryCount = sizeof(dbg_messages_entries) / sizeof(WGPUBindGroupEntry),
        .entries = dbg_messages_entries,
    };
    m_rv_dbg_messages_bind_group = wgpuDeviceCreateBindGroup(m_device, &dbg_messages_bind_group_desc);

    Result<ShaderV2> shader_maybe = ShaderV2::load("assets/shaders/rv_raytracing.wgsl");
    const std::string& shader_source = shader_maybe.value().get_source_code();

    WGPUShaderSourceWGSL shader_source_desc{
        .chain = {.next = nullptr, .sType = WGPUSType_ShaderSourceWGSL},
        .code = {.data = shader_source.data(), .length = shader_source.size()},
    };
    WGPUShaderModuleDescriptor shader_desc{
        .nextInChain = &shader_source_desc.chain,
        .label = WGPU_STRING_VIEW_INIT,
    };
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(m_device, &shader_desc);

    WGPUComputePipelineDescriptor pipeline_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .layout = m_rv_raytracing_pipeline_layout,
        .compute = WGPUProgrammableStageDescriptor{
            .nextInChain = nullptr,
            .module = shader_module,
            .entryPoint = WGPU_STRING_VIEW("main"),
            .constantCount = 0,
            .constants = nullptr,
        },
    };
    m_rv_raytracing_pipeline = wgpuDeviceCreateComputePipeline(m_device, &pipeline_desc);
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

WGPUSurface RVRenderer::create_surface(WGPUInstance instance, SDL_Window *window)
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

Result<ShaderV2> ShaderV2::load(const std::string& path)
{
    const Result<File> file_result = Filesystem::open_file(path);
    YEET(file_result);

    const auto shader_source = file_result->read_to_buffer();
    const auto parent_path = std::filesystem::path(path).parent_path();

    std::string final_source;
    std::string source(shader_source.data(), shader_source.size());

    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line))
    {
        if (line.starts_with("#include "))
        {
            auto s = line.substr(9);

            std::filesystem::path p = parent_path;
            p += "/" + s.substr(1, s.size() - 2);

            const Result<File> file_result = Filesystem::open_file(p);
            YEET(file_result);

            const auto include_source = file_result->read_to_buffer();
            final_source.append(include_source.data(), include_source.size());
            final_source.push_back('\n');
        }
        else if (line.starts_with("#"))
        {
            println(stderr, "Unknown preprocessor directive `{}`", line);
            return Error(ErrorKind::ShaderCompilationFailed);
        }
        else
        {
            final_source.append(line);
            final_source.push_back('\n');
        }
    }

    return ShaderV2(final_source);
}

ShaderV2::ShaderV2(const std::string& source)
    : m_source(source)
{
}
