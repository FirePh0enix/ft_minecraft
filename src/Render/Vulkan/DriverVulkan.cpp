#include "Render/Vulkan/DriverVulkan.hpp"
#include "Core/Containers/StackVector.hpp"
#include "Core/Logger.hpp"
#include "Render/Graph.hpp"
#include "Render/Shader.hpp"

#include <chrono>

#include <SDL3/SDL_vulkan.h>

#ifdef __has_debug_menu
#include <imgui.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

static inline vk::BufferUsageFlags convert_buffer_usage(BufferUsageFlags usage)
{
    vk::BufferUsageFlags flags{};

    if (usage.has_any(BufferUsageFlagBits::CopySource))
        flags |= vk::BufferUsageFlagBits::eTransferSrc;
    if (usage.has_any(BufferUsageFlagBits::CopyDest))
        flags |= vk::BufferUsageFlagBits::eTransferDst;
    if (usage.has_any(BufferUsageFlagBits::Uniform))
        flags |= vk::BufferUsageFlagBits::eUniformBuffer;
    if (usage.has_any(BufferUsageFlagBits::Index))
        flags |= vk::BufferUsageFlagBits::eIndexBuffer;
    if (usage.has_any(BufferUsageFlagBits::Vertex))
        flags |= vk::BufferUsageFlagBits::eVertexBuffer;

    return flags;
}

static inline vk::ImageUsageFlags convert_texture_usage(TextureUsageFlags usage)
{
    vk::ImageUsageFlags flags{};

    if (usage.has_any(TextureUsageFlagBits::CopySource))
        flags |= vk::ImageUsageFlagBits::eTransferSrc;
    if (usage.has_any(TextureUsageFlagBits::CopyDest))
        flags |= vk::ImageUsageFlagBits::eTransferDst;
    if (usage.has_any(TextureUsageFlagBits::Sampled))
        flags |= vk::ImageUsageFlagBits::eSampled;
    if (usage.has_any(TextureUsageFlagBits::ColorAttachment))
        flags |= vk::ImageUsageFlagBits::eColorAttachment;
    if (usage.has_any(TextureUsageFlagBits::DepthAttachment))
        flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    return flags;
}

static inline vk::Format convert_texture_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8Unorm:
        return vk::Format::eR8Unorm;
    case TextureFormat::RGBA8Unorm:
        return vk::Format::eR8G8B8A8Unorm;
    case TextureFormat::RGBA8Srgb:
        return vk::Format::eR8G8B8A8Srgb;
    case TextureFormat::BGRA8Srgb:
        return vk::Format::eB8G8R8A8Srgb;
    case TextureFormat::R32Sfloat:
        return vk::Format::eR32Sfloat;
    case TextureFormat::RG32Sfloat:
        return vk::Format::eR32G32Sfloat;
    case TextureFormat::RGB32Sfloat:
        return vk::Format::eR32G32B32Sfloat;
    case TextureFormat::RGBA32Sfloat:
        return vk::Format::eR32G32B32A32Sfloat;
    case TextureFormat::Depth32:
        return vk::Format::eD32Sfloat;
    }

    return vk::Format::eUndefined;
}

static inline vk::Format convert_shader_type(ShaderType type)
{
    switch (type)
    {
    case ShaderType::Float32:
        return vk::Format::eR32Sfloat;
    case ShaderType::Float32x2:
        return vk::Format::eR32G32Sfloat;
    case ShaderType::Float32x3:
        return vk::Format::eR32G32B32Sfloat;
    case ShaderType::Float32x4:
        return vk::Format::eR32G32B32A32Sfloat;
    case ShaderType::Uint32:
        return vk::Format::eR32Uint;
    case ShaderType::Uint32x2:
        return vk::Format::eR32G32Uint;
    case ShaderType::Uint32x3:
        return vk::Format::eR32G32B32Uint;
    case ShaderType::Uint32x4:
        return vk::Format::eR32G32B32A32Uint;
    }

    return {};
}

static inline vk::PolygonMode convert_polygon_mode(PolygonMode polygon_mode)
{
    switch (polygon_mode)
    {
    case PolygonMode::Fill:
        return vk::PolygonMode::eFill;
    case PolygonMode::Line:
        return vk::PolygonMode::eLine;
    case PolygonMode::Point:
        return vk::PolygonMode::ePoint;
    };

    return vk::PolygonMode::eFill;
}

static inline vk::CullModeFlags convert_cull_mode(CullMode cull_mode)
{
    switch (cull_mode)
    {
    case CullMode::Back:
        return vk::CullModeFlagBits::eBack;
    case CullMode::Front:
        return vk::CullModeFlagBits::eFront;
    case CullMode::None:
        return vk::CullModeFlagBits::eNone;
    };

    return vk::CullModeFlagBits::eNone;
}

static inline vk::IndexType convert_index_type(IndexType index_type)
{
    switch (index_type)
    {
    case IndexType::Uint16:
        return vk::IndexType::eUint16;
    case IndexType::Uint32:
        return vk::IndexType::eUint32;
    };

    return vk::IndexType::eUint16;
}

static vk::Filter convert_filter(Filter filter)
{
    switch (filter)
    {
    case Filter::Linear:
        return vk::Filter::eLinear;
    case Filter::Nearest:
        return vk::Filter::eNearest;
    }

    return vk::Filter::eLinear;
}

static vk::SamplerAddressMode convert_address_mode(AddressMode address_mode)
{
    switch (address_mode)
    {
    case AddressMode::Repeat:
        return vk::SamplerAddressMode::eRepeat;
    case AddressMode::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    }

    return vk::SamplerAddressMode::eRepeat;
}

static vk::ShaderStageFlags convert_shader_stage_mask(ShaderStageFlags shader_flags)
{
    vk::ShaderStageFlags flags{};

    if (shader_flags.has_any(ShaderStageFlagBits::Vertex))
        flags |= vk::ShaderStageFlagBits::eVertex;
    else if (shader_flags.has_any(ShaderStageFlagBits::Fragment))
        flags |= vk::ShaderStageFlagBits::eFragment;
    else if (shader_flags.has_any(ShaderStageFlagBits::Compute))
        flags |= vk::ShaderStageFlagBits::eCompute;

    return {};
}

static vk::ShaderStageFlagBits convert_shader_stage(ShaderStageFlagBits shader_stage)
{
    switch (shader_stage)
    {
    case ShaderStageFlagBits::Vertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderStageFlagBits::Fragment:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderStageFlagBits::Compute:
        return vk::ShaderStageFlagBits::eCompute;
    }
    return {};
}

static vk::ImageLayout convert_texture_layout(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::Undefined:
        return vk::ImageLayout::eUndefined;
    case TextureLayout::DepthStencilAttachment:
        return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case TextureLayout::CopyDst:
        return vk::ImageLayout::eTransferDstOptimal;
    case TextureLayout::ShaderReadOnly:
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    case TextureLayout::DepthStencilReadOnly:
        return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    }

    return {};
}

static vk::SamplerCreateInfo convert_sampler(SamplerDescriptor sampler)
{
    return vk::SamplerCreateInfo(
        {},
        convert_filter(sampler.mag_filter), convert_filter(sampler.min_filter),
        vk::SamplerMipmapMode::eLinear,
        convert_address_mode(sampler.address_mode.u), convert_address_mode(sampler.address_mode.v), convert_address_mode(sampler.address_mode.w),
        0.0,
        vk::False, 0.0,
        vk::False, vk::CompareOp::eEqual,
        0.0, 0.0,
        vk::BorderColor::eIntOpaqueBlack,
        vk::False);
}

Result<vk::Pipeline> PipelineCache::get_or_create(Ref<Material> material, vk::RenderPass render_pass, bool depth_pass, bool use_previous_depth_pass)
{
    ZoneScoped;

    PipelineCache::Key key{.material = material, .render_pass = render_pass, .depth_pass = depth_pass, .use_previous_depth_pass = use_previous_depth_pass};
    auto iter = std::find_if(m_pipelines.begin(), m_pipelines.end(), [material, render_pass, depth_pass, use_previous_depth_pass](auto p)
                             { return p.first.material.ptr() == material.ptr() && (VkRenderPass)p.first.render_pass == (VkRenderPass)render_pass && p.first.depth_pass == depth_pass && p.first.use_previous_depth_pass == use_previous_depth_pass; });

    if (iter != m_pipelines.end())
    {
        return iter->second;
    }
    else
    {
        Ref<MaterialLayoutVulkan> layout = material->get_layout().cast_to<MaterialLayoutVulkan>();

        // TODO: Add an in memory cache to store shader variants.
        // Ref<Shader> shader = depth_pass ? layout->m_shader->recompile(ShaderFlagBits::DepthPass, ShaderStageFlagBits::Vertex | ShaderStageFlagBits::Fragment).value() : layout->m_shader;

        auto pipeline_result = RenderingDriverVulkan::get()->create_graphics_pipeline(layout->m_shader, layout->m_instance_layout, layout->m_polygon_mode, layout->m_cull_mode, layout->m_flags, layout->m_pipeline_layout, render_pass, use_previous_depth_pass);
        YEET(pipeline_result);

        m_pipelines[key] = pipeline_result.value();
        return pipeline_result.value();
    }
}

Result<vk::Pipeline> PipelineCache::get_compute(const Ref<Material>& material)
{
    ZoneScoped;

    PipelineCache::Key key{.material = material, .render_pass = nullptr, .depth_pass = false, .use_previous_depth_pass = false};
    auto iter = std::find_if(m_pipelines.begin(), m_pipelines.end(), [material](auto p)
                             { return p.first.material.ptr() == material.ptr() && (VkRenderPass)p.first.render_pass == nullptr && p.first.depth_pass == false && p.first.use_previous_depth_pass == false; });

#ifdef __has_shader_hot_reload
    if (iter != m_pipelines.end())
    {
        // TODO:
        // - What if multiple material layouts are using the same shader ?

        Ref<Shader>& shader = iter->first.material->get_layout().cast_to<MaterialLayoutVulkan>()->m_shader;

        if (shader->was_reloaded())
        {
            shader->set_was_reloaded(false);

            m_pipelines.erase(key);
            return get_or_create(material, render_pass, depth_pass, use_previous_depth_pass);
        }
        else
        {
            return iter->second;
        }
    }
#else
    if (iter != m_pipelines.end())
    {
        return iter->second;
    }
#endif
    else
    {
        Ref<MaterialLayoutVulkan> layout = material->get_layout().cast_to<MaterialLayoutVulkan>();

        Ref<Shader> shader = layout->m_shader;
        Result<vk::Pipeline> pipeline_result = RenderingDriverVulkan::get()->create_compute_pipeline(shader, layout->m_pipeline_layout);
        YEET(pipeline_result);

        m_pipelines[key] = pipeline_result.value();
        return pipeline_result.value();
    }
}

Result<vk::Sampler> SamplerCache::get_or_create(SamplerDescriptor sampler)
{
    auto iter = m_samplers.find(sampler);

    if (iter != m_samplers.end())
    {
        return iter->second;
    }
    else
    {
        auto sampler_vk_result = RenderingDriverVulkan::get()->get_device().createSampler(convert_sampler(sampler));
        YEET_RESULT(sampler_vk_result);

        m_samplers[sampler] = sampler_vk_result.value;
        return sampler_vk_result.value;
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

RenderGraphCache::RenderPass& RenderGraphCache::set_render_pass(uint32_t index, RenderPassDescriptor desc)
{
    Result<vk::RenderPass> render_pass_result = RenderingDriverVulkan::get()->get_render_pass_cache().get_or_create(desc);

    if (index >= m_render_passes.size())
    {
        RenderGraphCache::RenderPass render_pass_cache{};
        render_pass_cache.name = desc.name;
        render_pass_cache.render_pass = render_pass_result.value();

        for (const auto& attachment : desc.color_attachments)
        {
            RenderGraphCache::Attachment attach{};
            attach.surface_texture = attachment.surface_texture;

            // FIXME: Textures should be recreated when resizing the window.
            Extent2D extent = RenderingDriver::get()->get_surface_extent();
            Result<Ref<Texture>> texture_result = RenderingDriver::get()->create_texture(extent.width, extent.height, attachment.format, TextureUsageFlagBits::ColorAttachment);
            attach.texture = texture_result.value().cast_to<TextureVulkan>();

            render_pass_cache.attachments.push_back(attach);
        }

        if (desc.depth_attachment.has_value())
        {
            RenderGraphCache::Attachment attach{};
            attach.depth_load_previous = desc.depth_attachment->load;

            if (!attach.depth_load_previous)
            {
                // FIXME: Textures should be recreated when resizing the window.
                // TODO: Should depth texture be hardcoded here ?
                Extent2D extent = RenderingDriver::get()->get_surface_extent();
                Result<Ref<Texture>> texture_result = RenderingDriver::get()->create_texture(extent.width, extent.height, TextureFormat::Depth32, TextureUsageFlagBits::DepthAttachment);
                attach.texture = texture_result.value().cast_to<TextureVulkan>();
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

Result<vk::RenderPass> RenderPassCache::get_or_create(RenderPassDescriptor desc)
{
    auto pair = m_render_passes.find(desc);

    if (pair != m_render_passes.end())
    {
        return pair->second;
    }
    else
    {
        constexpr size_t max_attachments = 3;

        StackVector<vk::AttachmentDescription, max_attachments> descriptions;
        StackVector<vk::AttachmentReference, max_attachments> references;

        uint32_t index = 0;

        for (const auto& attachment : desc.color_attachments)
        {
            const vk::AttachmentDescription attach(
                {},
                attachment.surface_texture ? RenderingDriverVulkan::get()->get_surface_format() : convert_texture_format(attachment.format), vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, attachment.surface_texture ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eColorAttachmentOptimal);
            descriptions.push_back(attach);

            const vk::AttachmentReference ref(index, vk::ImageLayout::eColorAttachmentOptimal);
            references.push_back(ref);

            index += 1;
        }

        vk::AttachmentReference depth_ref(index, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        if (desc.depth_attachment.has_value())
        {
            const auto& attachment = desc.depth_attachment.value();

            const vk::AttachmentDescription attach(
                {},
                convert_texture_format(TextureFormat::Depth32), vk::SampleCountFlagBits::e1,
                attachment.load ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear, attachment.save ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                attachment.load ? vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal : vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            descriptions.push_back(attach);
        }

        vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, references, {}, desc.depth_attachment.has_value() ? &depth_ref : nullptr);

        StackVector<vk::SubpassDependency, 1> dependencies;
        dependencies.push_back(vk::SubpassDependency(vk::SubpassExternal, 0,
                                                     vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                     vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite));

        auto render_pass_result = RenderingDriverVulkan::get()->get_device().createRenderPass(vk::RenderPassCreateInfo({}, descriptions, {subpass}, dependencies));
        YEET_RESULT(render_pass_result);

        m_render_passes[desc] = render_pass_result.value;

        return render_pass_result.value;
    }
}

Result<vk::Framebuffer> FramebufferCache::get_or_create(FramebufferCache::Key key)
{
    auto pair = m_framebuffers.find(key);

    if (pair != m_framebuffers.end())
    {
        return pair->second;
    }
    else
    {
        vk::FramebufferCreateInfo framebuffer_info({}, key.render_pass, key.views, key.width, key.height, 1);
        auto result = RenderingDriverVulkan::get()->get_device().createFramebuffer(framebuffer_info);
        YEET_RESULT(result);

        m_framebuffers[key] = result.value;

        return result.value;
    }
}

void FramebufferCache::clear_with_size(uint32_t width, uint32_t height)
{
    (void)width;
    (void)height;
    // FIXME: This will leak memory but prevent a segmentation fault.
    // std::map<Key, vk::Framebuffer>::iterator iter;

    // while (true)
    // {
    //     iter = std::find_if(m_framebuffers.begin(), m_framebuffers.end(), [width, height](const std::pair<Key, vk::Framebuffer>& pair)
    //                         { return pair.first.width == width && pair.first.height == height; });
    //     if (iter == m_framebuffers.end())
    //         break;
    //     RenderingDriverVulkan::get()->get_device().destroyFramebuffer(iter->second);
    // }
}

void FramebufferCache::clear_with_renderpass(vk::RenderPass render_pass)
{
    std::map<Key, vk::Framebuffer>::iterator iter;

    while (true)
    {
        iter = std::find_if(m_framebuffers.begin(), m_framebuffers.end(), [render_pass](const std::pair<Key, vk::Framebuffer>& pair)
                            { return (VkRenderPass)pair.first.render_pass == (VkRenderPass)render_pass; });
        if (iter == m_framebuffers.end())
            break;
        RenderingDriverVulkan::get()->get_device().destroyFramebuffer(iter->second);
    }
}

RenderingDriverVulkan::RenderingDriverVulkan()
{
}

RenderingDriverVulkan::~RenderingDriverVulkan()
{
    if (m_device)
    {
#ifdef __has_debug_menu
        ImGui::DestroyContext();
#endif

        destroy_swapchain();

        m_device.destroyQueryPool(m_timestamp_query_pool);

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            m_device.destroySemaphore(m_acquire_semaphores[i]);
            m_device.destroySemaphore(m_submit_semaphores[i]);
            m_device.destroyFence(m_frame_fences[i]);
        }

        m_device.freeCommandBuffers(m_graphics_command_pool, m_command_buffers);
        m_device.destroyCommandPool(m_graphics_command_pool);

#ifndef __platform_macos
        // tracy::DestroyVkContext(m_tracy_context);
#endif

        m_device.destroy();
    }

    if (m_instance)
    {
        m_instance.destroySurfaceKHR(m_surface);
        m_instance.destroy();
    }
}

Result<Ref<Buffer>> StagingBufferPool::get_or_create(size_t size, BufferUsageFlags usage, BufferVisibility visibility)
{
    Key key{.size = size, .usage = usage, .visibility = visibility};
    auto iter = m_buffers.find(key);

    if (iter != m_buffers.end())
    {
        return iter->second;
    }
    else
    {
        auto result = RenderingDriver::get()->create_buffer(size, usage, visibility);
        YEET(result);

        Ref<Buffer> buffer = result.value();
        return buffer;
    }
}

Result<> RenderingDriverVulkan::initialize(const Window& window, bool enable_validation)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    Uint32 instance_extensions_count = 0;
    auto instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extensions_count);

    vk::ApplicationInfo app_info("ft_minecraft", 0, "No engine", 0, VK_API_VERSION_1_2);

    std::vector<const char *> validation_layers;

    if (enable_validation)
    {
        validation_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    std::vector<const char *> required_instance_extensions(instance_extensions_count);
    for (size_t i = 0; i < instance_extensions_count; i++)
        required_instance_extensions[i] = (char *)instance_extensions[i];

    required_instance_extensions.push_back("VK_KHR_get_physical_device_properties2");

#ifndef __platform_macos
    vk::InstanceCreateFlags instance_flags = {};
#else
    vk::InstanceCreateFlags instance_flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    auto instance_result = vk::createInstance(vk::InstanceCreateInfo(instance_flags, &app_info, validation_layers, required_instance_extensions));
    YEET_RESULT(instance_result);
    m_instance = instance_result.value;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(window.get_window_ptr(), m_instance, nullptr, &surface))
        return Error(ErrorKind::BadDriver);
    m_surface = vk::SurfaceKHR(surface);

    // Select the best physical device
    // TODO: Add this to device pick.
    // vk::PhysicalDeviceFeatures required_features = {};
    // vk::PhysicalDeviceFeatures optional_features = {};

    std::vector<const char *> required_extensions;
    std::vector<const char *> optional_extensions;

    required_extensions.push_back("VK_KHR_swapchain");
    required_extensions.push_back("VK_EXT_host_query_reset");

#ifdef TRACY_ENABLE
    // This is widely supported except for android. This is only needed when profiling with tracy anyway.
    // required_extensions.push_back("VK_EXT_calibrated_timestamps");
#endif

#ifdef __platform_macos
    required_extensions.push_back("VK_KHR_portability_subset");
#endif

    vk::PhysicalDeviceHostQueryResetFeatures host_query_reset_features{};

#ifdef __DEBUG__
    host_query_reset_features.hostQueryReset = vk::True;
#endif

#ifdef __platform_macos
    vk::PhysicalDevicePortabilitySubsetFeaturesKHR portability_subset_features;
    portability_subset_features.imageViewFormatSwizzle = vk::True;

    host_query_reset_features.pNext = &portability_subset_features;
#endif

    std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices().value;
    std::optional<PhysicalDeviceWithInfo> physical_device_with_info_result = pick_best_device(physical_devices, required_extensions, optional_extensions);

    if (!physical_device_with_info_result.has_value())
        return Error(ErrorKind::NoSuitableDevice);

    m_physical_device = physical_device_with_info_result->physical_device;
    m_physical_device_properties = physical_device_with_info_result->properties;
    m_surface_format = physical_device_with_info_result->surface_format;

    info("GPU selected: {}", m_physical_device_properties.deviceName.data());

    auto surface_capabilities_result = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    if (surface_capabilities_result.result != vk::Result::eSuccess)
        return Error(surface_capabilities_result.result);
    m_surface_capabilities = surface_capabilities_result.value;

    auto surface_present_modes_result = m_physical_device.getSurfacePresentModesKHR(m_surface);
    if (surface_present_modes_result.result != vk::Result::eSuccess)
        return Error(surface_present_modes_result.result);
    m_surface_present_modes = surface_present_modes_result.value;

    // Create the actual device used to interact with vulkan
    m_graphics_queue_index = physical_device_with_info_result->queue_info.graphics_index.value();
    m_compute_queue_index = physical_device_with_info_result->queue_info.compute_index.value();

    float queue_priority = 1.0f;
    std::array<vk::DeviceQueueCreateInfo, 2> queue_infos{
        vk::DeviceQueueCreateInfo({}, m_graphics_queue_index, 1, &queue_priority),
        vk::DeviceQueueCreateInfo({}, m_compute_queue_index, 1, &queue_priority),
    };

    std::vector<const char *> device_extensions;
    device_extensions.reserve(required_extensions.size() + optional_extensions.size());

    for (const auto& ext : required_extensions)
        device_extensions.push_back(ext);
    for (const auto& ext : optional_extensions)
        device_extensions.push_back(ext);

    // TODO: device features
    vk::PhysicalDeviceFeatures device_features{};
    device_features.fillModeNonSolid = true;

    auto device_result = m_physical_device.createDevice(vk::DeviceCreateInfo({}, queue_infos.size(), queue_infos.data(), validation_layers.size(), validation_layers.data(), device_extensions.size(), device_extensions.data(), &device_features, &host_query_reset_features));
    YEET_RESULT(device_result);
    m_device = device_result.value;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);

    m_graphics_queue = m_device.getQueue(m_graphics_queue_index, 0);
    m_compute_queue = m_device.getQueue(m_compute_queue_index, 0);

    // Allocate enough command buffers and synchronization primitives for each frame in flight.
    auto gcp_result = m_device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_queue_index));
    YEET_RESULT(gcp_result);
    m_graphics_command_pool = gcp_result.value;

    auto buffer_alloc_result = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(m_graphics_command_pool, vk::CommandBufferLevel::ePrimary, max_frames_in_flight));
    YEET_RESULT(buffer_alloc_result);

    for (size_t i = 0; i < max_frames_in_flight; i++)
    {
        m_acquire_semaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo()).value;
        m_frame_fences[i] = m_device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)).value;
        m_command_buffers[i] = buffer_alloc_result.value[i];
    }

    m_swapchain_image_count = m_surface_capabilities.maxImageCount == 0 ? m_surface_capabilities.minImageCount + 1 : std::min(m_surface_capabilities.maxImageCount, m_surface_capabilities.minImageCount + 1);

    // We need one submit semaphore per swapchain images.
    m_submit_semaphores.resize(m_swapchain_image_count);

    for (size_t i = 0; i < m_swapchain_image_count; i++)
    {
        m_submit_semaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo()).value;
    }

    auto transfer_buffer_result = m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(m_graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1));
    YEET_RESULT(transfer_buffer_result);
    m_transfer_buffer = transfer_buffer_result.value[0];

    m_timestamp_query_pool = m_device.createQueryPool(vk::QueryPoolCreateInfo({}, vk::QueryType::eTimestamp, max_frames_in_flight * 2)).value;
    for (size_t i = 0; i < max_frames_in_flight; i++)
        m_device.resetQueryPool(m_timestamp_query_pool, i * 2, 2);

    m_memory_properties = m_physical_device.getMemoryProperties();

    YEET(configure_surface(window, VSync::Off));

#ifndef __platform_macos
    // This uses `VK_EXT_host_query_reset` & `VK_EXT_calibrated_timestamps`
    // m_tracy_context = TracyVkContextHostCalibrated(m_instance, m_physical_device, m_device, vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr, vk::detail::defaultDispatchLoaderDynamic.vkGetDeviceProcAddr);
#endif

    m_window = window.get_window_ptr();
    m_start_time = std::chrono::high_resolution_clock::now();

    return 0;
}

Result<> RenderingDriverVulkan::initialize_imgui()
{
#ifdef __has_debug_menu
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    bool res = ImGui_ImplSDL3_InitForVulkan(m_window);
    if (!res)
        return Error(ErrorKind::ImGuiInitFailed);

    ImGui_ImplVulkan_InitInfo init_info{
        .ApiVersion = VK_API_VERSION_1_2,
        .Instance = m_instance,
        .PhysicalDevice = m_physical_device,
        .Device = m_device,
        .QueueFamily = m_graphics_queue_index,
        .Queue = m_graphics_queue,
        .DescriptorPool = VK_NULL_HANDLE,
        .RenderPass = (VkRenderPass)m_render_graph_cache.get_render_pass(-1).render_pass,
        .MinImageCount = max_frames_in_flight,
        .ImageCount = max_frames_in_flight,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache = VK_NULL_HANDLE,
        .Subpass = 0,
        .DescriptorPoolSize = 16,
        .UseDynamicRendering = false,
        .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{},
        .Allocator = nullptr,
        .CheckVkResultFn = nullptr,
        .MinAllocationSize = 0,
    };

    res = ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_2, imgui_get_proc_addr);
    if (!res)
        return Error(ErrorKind::ImGuiInitFailed);

    res = ImGui_ImplVulkan_Init(&init_info);
    if (!res)
        return Error(ErrorKind::ImGuiInitFailed);

    return 0;
#else
    return 0;
#endif
}

Result<> RenderingDriverVulkan::configure_surface(const Window& window, VSync vsync)
{
    (void)m_device.waitIdle();

    vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;

    switch (vsync)
    {
    case VSync::Off:
        present_mode = vk::PresentModeKHR::eImmediate;
        break;
    case VSync::On:
        present_mode = vk::PresentModeKHR::eFifoRelaxed;
        break;
    }

    // According to the vulkan spec only FIFO is required to be supported so we fallback on that if other modes are not supported.
    if (std::find(m_surface_present_modes.begin(), m_surface_present_modes.end(), present_mode) == m_surface_present_modes.end())
        present_mode = vk::PresentModeKHR::eFifo;

    const auto& size = window.size();

    // Query new surface capabilities
    auto surface_capabilities_result = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    if (surface_capabilities_result.result != vk::Result::eSuccess)
        return Error(surface_capabilities_result.result);
    m_surface_capabilities = surface_capabilities_result.value;

    const vk::Extent2D surface_extent(std::clamp(size.width, m_surface_capabilities.minImageExtent.width, m_surface_capabilities.maxImageExtent.width),
                                      std::clamp(size.height, m_surface_capabilities.minImageExtent.height, m_surface_capabilities.maxImageExtent.height));

    auto swapchain_result = m_device.createSwapchainKHR(vk::SwapchainCreateInfoKHR(
        {},
        m_surface,
        m_swapchain_image_count,
        m_surface_format.format, m_surface_format.colorSpace,
        surface_extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0, nullptr,
        m_surface_capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode,
        vk::True,
        m_swapchain));
    YEET_RESULT(swapchain_result);

    auto swapchain_images_result = m_device.getSwapchainImagesKHR(swapchain_result.value);
    YEET_RESULT(swapchain_images_result);

    std::vector<Ref<Texture>> swapchain_textures;
    swapchain_textures.reserve(swapchain_images_result.value.size());

    std::vector<vk::Framebuffer> swapchain_framebuffers;
    swapchain_framebuffers.reserve(swapchain_images_result.value.size());

    for (const auto& swapchain_image : swapchain_images_result.value)
    {
        auto texture_result = create_texture_from_vk_image(swapchain_image, surface_extent.width, surface_extent.height, m_surface_format.format);
        YEET(texture_result);
        swapchain_textures.push_back(texture_result.value());
    }

    destroy_swapchain();

    m_swapchain = swapchain_result.value;
    m_swapchain_images = std::move(swapchain_images_result.value);
    m_swapchain_textures = std::move(swapchain_textures);
    m_surface_extent = Extent2D(surface_extent.width, surface_extent.height);

    return 0;
}

void RenderingDriverVulkan::destroy_swapchain()
{
    if (m_swapchain)
    {
        m_framebuffer_cache.clear_with_size(m_surface_extent.width, m_surface_extent.height);

        m_swapchain_textures.clear();

        m_device.destroySwapchainKHR(m_swapchain);
    }
}

void RenderingDriverVulkan::poll()
{
#ifdef __has_shader_hot_reload
    for (auto& shader : Shader::shaders)
    {
        shader->reload_if_needed();
    }
#endif

#ifdef __has_debug_menu
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
#endif
}

void RenderingDriverVulkan::limit_frames(uint32_t limit)
{
    m_frames_limit = limit;
    m_time_between_frames = 1'000'000 / m_frames_limit;
}

Result<Ref<Buffer>> RenderingDriverVulkan::create_buffer(size_t size, BufferUsageFlags usage, BufferVisibility visibility)
{
    vk::MemoryPropertyFlags memory_properties{};

    switch (visibility)
    {
    case BufferVisibility::GPUOnly:
        memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        break;
    case BufferVisibility::GPUAndCPU:
        memory_properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
        break;
    }

    auto buffer_result = m_device.createBuffer(vk::BufferCreateInfo({}, size, convert_buffer_usage(usage)));
    YEET_RESULT(buffer_result);

    auto memory_result = allocate_memory_for_buffer(buffer_result.value, memory_properties);
    YEET(memory_result);

    return make_ref<BufferVulkan>(buffer_result.value, memory_result.value(), size).cast_to<Buffer>();
}

void RenderingDriverVulkan::update_buffer(const Ref<Buffer>& dest, View<uint8_t> view, size_t offset)
{
    ZoneScoped;

    ERR_COND_VR(view.size() > dest->size() - offset, "too big: {} but size is {}", view.size(), dest->size() - offset);

    if (view.size() == 0)
    {
        return;
    }

    auto buffer_result = create_buffer(view.size(), BufferUsageFlagBits::CopySource, BufferVisibility::GPUAndCPU);
    ERR_EXPECT_R(buffer_result, "failed to create the staging buffer");

    Ref<BufferVulkan> staging_buffer = buffer_result.value().cast_to<BufferVulkan>();

    // Copy the data into the staging buffer.
    auto map_result = RenderingDriverVulkan::get()->get_device().mapMemory(staging_buffer->memory, 0, view.size(), {});
    ERR_RESULT_RET(map_result);
    std::memcpy(map_result.value, view.data(), view.size());
    RenderingDriverVulkan::get()->get_device().unmapMemory(staging_buffer->memory);

    // RenderGraph::get().add_copy(staging_buffer, dest, view.size(), 0, offset);

    // Copy from the staging buffer to the final buffer
    std::lock_guard<std::mutex> lock(RenderingDriverVulkan::get()->get_graphics_mutex());
    vk::CommandBuffer cb = RenderingDriverVulkan::get()->get_transfer_buffer();

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    vk::BufferCopy region(0, offset, std::min(view.size(), dest->size() - offset));

    cb.copyBuffer(staging_buffer->buffer, dest.cast_to<BufferVulkan>()->buffer, {region});
    ERR_RESULT_E_RET(cb.end());

    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().submit({vk::SubmitInfo(0, nullptr, nullptr, 1, &cb)}));
    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().waitIdle());
}

Result<Ref<Texture>> RenderingDriverVulkan::create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage)
{
    const vk::Format vk_format = convert_texture_format(format);

    auto image_result = m_device.createImage(vk::ImageCreateInfo(
        {},
        vk::ImageType::e2D,
        vk_format,
        vk::Extent3D(width, height, 1),
        1, // mipLevels
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        convert_texture_usage(usage),
        vk::SharingMode::eExclusive));
    YEET_RESULT(image_result);

    auto memory_result = allocate_memory_for_image(image_result.value, vk::MemoryPropertyFlagBits::eDeviceLocal);
    YEET(memory_result);

    // TODO: format_to_aspect_mask()
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::Depth32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::e2D, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, 1, true).cast_to<Texture>();
}

Result<Ref<Texture>> RenderingDriverVulkan::create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage, uint32_t layers)
{
    const vk::Format vk_format = convert_texture_format(format);

    auto image_result = m_device.createImage(vk::ImageCreateInfo(
        {},
        vk::ImageType::e2D,
        vk_format,
        vk::Extent3D(width, height, 1),
        1, // mipLevels
        layers,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        convert_texture_usage(usage),
        vk::SharingMode::eExclusive));
    YEET_RESULT(image_result);

    auto memory_result = allocate_memory_for_image(image_result.value, vk::MemoryPropertyFlagBits::eDeviceLocal);
    YEET(memory_result);

    // TODO: format_to_aspect_mask()
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::Depth32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::e2DArray, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, layers)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, layers, true).cast_to<Texture>();
}

Result<Ref<Texture>> RenderingDriverVulkan::create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage)
{
    const vk::Format vk_format = convert_texture_format(format);

    auto image_result = m_device.createImage(vk::ImageCreateInfo(
        vk::ImageCreateFlagBits::eCubeCompatible,
        vk::ImageType::e2D,
        vk_format,
        vk::Extent3D(width, height, 1),
        1, // mipLevels
        6,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        convert_texture_usage(usage),
        vk::SharingMode::eExclusive));
    YEET_RESULT(image_result);

    auto memory_result = allocate_memory_for_image(image_result.value, vk::MemoryPropertyFlagBits::eDeviceLocal);
    YEET(memory_result);

    // TODO: format_to_aspect_mask()
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::Depth32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::eCube, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, 1, true).cast_to<Texture>();
}

Result<Ref<Mesh>> RenderingDriverVulkan::create_mesh(IndexType index_type, View<uint8_t> indices, View<glm::vec3> vertices, View<glm::vec2> uvs, View<glm::vec3> normals)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    auto index_buffer_result = create_buffer(indices.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Index);
    YEET(index_buffer_result);
    Ref<Buffer> index_buffer = index_buffer_result.value();

    auto vertex_buffer_result = create_buffer(vertices.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    YEET(vertex_buffer_result);
    Ref<Buffer> vertex_buffer = vertex_buffer_result.value();

    auto normal_buffer_result = create_buffer(normals.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    YEET(normal_buffer_result);
    Ref<Buffer> normal_buffer = normal_buffer_result.value();

    auto uv_buffer_result = create_buffer(uvs.size() * sizeof(glm::vec2), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    YEET(uv_buffer_result);
    Ref<Buffer> uv_buffer = uv_buffer_result.value();

    RenderingDriver::get()->update_buffer(index_buffer, indices.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(vertex_buffer, vertices.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(normal_buffer, normals.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(uv_buffer, uvs.as_bytes(), 0);

    return make_ref<MeshVulkan>(index_type, convert_index_type(index_type), vertex_count, index_buffer, vertex_buffer, normal_buffer, uv_buffer).cast_to<Mesh>();
}

Result<Ref<MaterialLayout>> RenderingDriverVulkan::create_material_layout(Ref<Shader> shader, MaterialFlags flags, std::optional<InstanceLayout> instance_layout, CullMode cull_mode, PolygonMode polygon_mode)
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(shader->get_bindings().size());

    for (const auto& binding_pair : shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        // TODO: Support multiple groups ?
        if (binding.group != 0)
        {
            continue;
        }

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            // FIXME: This assume that a texture sampler is 1 binding after the texture.

            bindings.push_back(vk::DescriptorSetLayoutBinding(binding.binding, vk::DescriptorType::eCombinedImageSampler, 1, convert_shader_stage_mask(binding.shader_stage), nullptr));
        }
        break;
        case BindingKind::UniformBuffer:
        {
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding.binding, vk::DescriptorType::eUniformBuffer, 1, convert_shader_stage_mask(binding.shader_stage), nullptr));
        }
        break;
        }
    }

    auto layout_result = RenderingDriverVulkan::get()->get_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, bindings));
    YEET_RESULT(layout_result);

    auto pool_result = DescriptorPool::create(layout_result.value, shader);
    YEET(pool_result);

    std::array<vk::PushConstantRange, 1> push_constant_ranges{
        vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)),
    };
    std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts{layout_result.value};

    auto pipeline_layout_result = RenderingDriverVulkan::get()->get_device().createPipelineLayout(vk::PipelineLayoutCreateInfo({}, descriptor_set_layouts, push_constant_ranges));
    YEET_RESULT(pipeline_layout_result);

    return make_ref<MaterialLayoutVulkan>(layout_result.value, pool_result.value(), shader, instance_layout, convert_polygon_mode(polygon_mode), convert_cull_mode(cull_mode), flags, pipeline_layout_result.value).cast_to<MaterialLayout>();
}

Result<Ref<MaterialLayout>> RenderingDriverVulkan::create_compute_material_layout(Ref<Shader> shader)
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(shader->get_bindings().size());

    for (const auto& binding_pair : shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        // TODO: Support multiple groups ?
        if (binding.group != 0)
        {
            continue;
        }

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            // FIXME: This assume that a texture sampler is 1 binding after the texture.
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding.binding, vk::DescriptorType::eCombinedImageSampler, 1, convert_shader_stage_mask(binding.shader_stage), nullptr));
        }
        break;
        case BindingKind::UniformBuffer:
        {
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding.binding, vk::DescriptorType::eUniformBuffer, 1, convert_shader_stage_mask(binding.shader_stage), nullptr));
        }
        break;
        case BindingKind::PushConstants:
            break;
        }
    }

    auto layout_result = RenderingDriverVulkan::get()->get_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, bindings));
    YEET_RESULT(layout_result);

    auto pool_result = DescriptorPool::create(layout_result.value, shader);
    YEET(pool_result);

    std::array<vk::PushConstantRange, 1> push_constant_ranges{
        vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)),
    };
    std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts{layout_result.value};

    auto pipeline_layout_result = RenderingDriverVulkan::get()->get_device().createPipelineLayout(vk::PipelineLayoutCreateInfo({}, descriptor_set_layouts, push_constant_ranges));
    YEET_RESULT(pipeline_layout_result);

    return make_ref<MaterialLayoutVulkan>(layout_result.value, pool_result.value(), shader, std::nullopt, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, MaterialFlags{}, pipeline_layout_result.value).cast_to<MaterialLayout>();
}

Result<Ref<Material>> RenderingDriverVulkan::create_material(const Ref<MaterialLayout>& layout)
{
    Ref<MaterialLayoutVulkan> layout_vk = layout.cast_to<MaterialLayoutVulkan>();

    auto set_result = layout_vk->m_descriptor_pool.allocate();
    YEET(set_result);

    return make_ref<MaterialVulkan>(layout, set_result.value()).cast_to<Material>();
}

void RenderingDriverVulkan::draw_graph(const RenderGraph& graph)
{
    constexpr uint64_t timeout = 500'000'000; // 500 ms

    // Limit frame when `m_frames_limit != 0`.
    if (m_frames_limit > 0)
    {
        uint64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_start_time).count();

        if (elapsed < m_time_between_frames)
            return;

        m_last_frame_limit_time = std::chrono::high_resolution_clock::now();
    }

    vk::Fence frame_fence = m_frame_fences[m_current_frame];

    ERR_RESULT_E_RET(m_device.waitForFences({frame_fence}, true, timeout));
    ERR_RESULT_E_RET(m_device.resetFences({frame_fence}));

    vk::Semaphore acquire_semaphore = m_acquire_semaphores[m_current_frame];

    auto image_index_result = m_device.acquireNextImageKHR(m_swapchain, timeout, acquire_semaphore, nullptr);

    switch (image_index_result.result)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
    case vk::Result::eErrorOutOfDateKHR:
        // TODO: Recreate the swapchain
        break;
    default:
        return;
    }

    ZoneScoped;

    uint32_t image_index = image_index_result.value;
    vk::CommandBuffer cb = m_command_buffers[m_current_frame];

    // Make sure we are not recording and submitting to the graphics from multiple threads at the same time.
    // TODO: The lock could be moved right before submission if the transfer command buffer was allocated from
    //       a different VkCommandPool.
    std::lock_guard<std::mutex> lock(RenderingDriverVulkan::get()->get_graphics_mutex());

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    // TracyVkZone(m_tracy_context, cb, "DrawMesh");

    // for (const auto& copy_instruction : graph.get_copy_instructions())
    // {
    //     Ref<BufferVulkan> src_buffer = copy_instruction.src.cast_to<BufferVulkan>();
    //     Ref<BufferVulkan> dst_buffer = copy_instruction.dst.cast_to<BufferVulkan>();
    //     vk::BufferCopy copy_region(copy_instruction.src_offset, copy_instruction.dst_offset, copy_instruction.size);
    //     cb.copyBuffer(src_buffer->buffer, dst_buffer->buffer, {copy_region});
    // }

    // vk::MemoryBarrier barrier(vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eMemoryRead);
    // cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlagBits::eByRegion, {barrier}, {}, {});

    vk::RenderPass current_render_pass;
    uint32_t render_pass_index = 0;
    bool depth_pass = false;
    bool use_previous_depth_pass = false;

    for (const auto& instruction : graph.get_instructions())
    {
        if (std::holds_alternative<BeginRenderPassInstruction>(instruction))
        {
            ZoneScopedN("draw_graph.begin_render_pass");

            const BeginRenderPassInstruction& begin_render_pass = std::get<BeginRenderPassInstruction>(instruction);
            const RenderGraphCache::RenderPass& render_pass_cache = m_render_graph_cache.set_render_pass(render_pass_index, begin_render_pass.descriptor);
            current_render_pass = render_pass_cache.render_pass;

            // std::vector<vk::ClearValue> clear_values;
            // clear_values.reserve(begin_render_pass.descriptor.color_attachments.size() + (size_t)begin_render_pass.descriptor.depth_attachment.has_value());

            StackVector<vk::ClearValue, 4> clear_values;

            for (const auto& attachment : begin_render_pass.descriptor.color_attachments)
            {
                (void)attachment;
                clear_values.push_back(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (begin_render_pass.descriptor.depth_attachment.has_value())
            {
                clear_values.push_back(vk::ClearDepthStencilValue(1.0));
            }

            std::vector<vk::ImageView> views;
            views.reserve(begin_render_pass.descriptor.color_attachments.size() + (size_t)begin_render_pass.descriptor.depth_attachment.has_value());

            // Collect attachment's imageview to query a framebuffer from the cache.
            for (const auto& attachment : render_pass_cache.attachments)
            {
                if (attachment.surface_texture)
                {
                    views.push_back(m_swapchain_textures[image_index].cast_to<TextureVulkan>()->image_view);
                }
                else
                {
                    views.push_back(attachment.texture->image_view);
                }
            }

            if (render_pass_cache.depth_attachment.has_value())
            {
                // Depth attachment can be loaded from a previous render pass.
                if (!render_pass_cache.depth_attachment->depth_load_previous)
                {
                    views.push_back(render_pass_cache.depth_attachment->texture->image_view);
                }
                else
                {
                    views.push_back(m_render_graph_cache.get_render_pass((int32_t)render_pass_index - 1).depth_attachment->texture->image_view);
                }
            }

            if (Result<vk::Framebuffer> framebuffer = m_framebuffer_cache.get_or_create({.views = views, .render_pass = current_render_pass, .width = m_surface_extent.width, .height = m_surface_extent.height}))
            {
                cb.beginRenderPass(vk::RenderPassBeginInfo(current_render_pass, framebuffer.value(), vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(m_surface_extent.width, m_surface_extent.height)), clear_values), vk::SubpassContents::eInline);
            }
            else
            {
                error("Framebuffer creation failed");
            }

            use_previous_depth_pass = begin_render_pass.descriptor.depth_attachment.has_value() && begin_render_pass.descriptor.depth_attachment->load;
            depth_pass = render_pass_cache.attachments.size() == 0; // we assume its a depth-only pass if there is no color attachments
            render_pass_index += 1;
        }
        else if (std::holds_alternative<EndRenderPassInstruction>(instruction))
        {
            cb.endRenderPass();

            depth_pass = false;
            use_previous_depth_pass = false;
        }
        else if (std::holds_alternative<DrawInstruction>(instruction))
        {
            ZoneScopedN("draw_graph.draw");

            const DrawInstruction& draw = std::get<DrawInstruction>(instruction);

            Ref<MeshVulkan> mesh = draw.mesh.cast_to<MeshVulkan>();
            Ref<MaterialVulkan> material = draw.material.cast_to<MaterialVulkan>();

            const Ref<MaterialLayoutVulkan>& material_layout = material->get_layout().cast_to<MaterialLayoutVulkan>();

            std::optional<Ref<Buffer>> instance_buffer = draw.instance_buffer;

            Result<vk::Pipeline> pipeline_result = m_pipeline_cache.get_or_create(material, current_render_pass, depth_pass, use_previous_depth_pass);
            ERR_EXPECT_B(pipeline_result, "Pipeline creation failed");

            cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_result.value());
            cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, material_layout->m_pipeline_layout, 0, {material->descriptor_set}, {});

            Ref<BufferVulkan> index_buffer = mesh->index_buffer.cast_to<BufferVulkan>();
            Ref<BufferVulkan> vertex_buffer = mesh->vertex_buffer.cast_to<BufferVulkan>();
            Ref<BufferVulkan> normal_buffer = mesh->normal_buffer.cast_to<BufferVulkan>();
            Ref<BufferVulkan> uv_buffer = mesh->uv_buffer.cast_to<BufferVulkan>();

            cb.bindIndexBuffer(index_buffer->buffer, 0, mesh->index_type_vk);
            cb.bindVertexBuffers(0, {vertex_buffer->buffer, normal_buffer->buffer, uv_buffer->buffer}, {0, 0, 0});

            if (instance_buffer.has_value())
            {
                cb.bindVertexBuffers(3, {(instance_buffer.value().cast_to<BufferVulkan>())->buffer}, {0});
            }

            cb.setViewport(0, {vk::Viewport(0.0, 0.0, (float)m_surface_extent.width, (float)m_surface_extent.height, 0.0, 1.0)});
            cb.setScissor(0, {vk::Rect2D({0, 0}, {m_surface_extent.width, m_surface_extent.height})});

            PushConstants push_constants{
                .view_matrix = draw.view_matrix,
                .nan = 0.0f / 0.0f,
            };

            cb.pushConstants(material_layout->m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constants);

            cb.drawIndexed(mesh->vertex_count(), draw.instance_count, 0, 0, 0);
        }
        else if (std::holds_alternative<ImGuiDrawInstruction>(instruction))
        {
#ifdef __has_debug_menu
            ZoneScopedN("draw_graph.imgui");

            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
#endif
        }
        else if (std::holds_alternative<BindMaterialInstruction>(instruction))
        {
            const BindMaterialInstruction& bind = std::get<BindMaterialInstruction>(instruction);

            Ref<MaterialVulkan> material = bind.material.cast_to<MaterialVulkan>();
            const Ref<MaterialLayoutVulkan>& material_layout = material->get_layout().cast_to<MaterialLayoutVulkan>();

            Result<vk::Pipeline> pipeline_result = m_pipeline_cache.get_or_create(material, current_render_pass, depth_pass, use_previous_depth_pass);
            ERR_EXPECT_B(pipeline_result, "Pipeline creation failed");

            cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_result.value());
            cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, material_layout->m_pipeline_layout, 0, {material->descriptor_set}, {});
        }
        else if (std::holds_alternative<BindIndexBufferInstruction>(instruction))
        {
            const BindIndexBufferInstruction& bind = std::get<BindIndexBufferInstruction>(instruction);
            const Ref<BufferVulkan>& buffer = bind.buffer.cast_to<BufferVulkan>();

            cb.bindIndexBuffer(buffer->buffer, 0, convert_index_type(bind.index_type));
        }
        else if (std::holds_alternative<BindIndexBufferInstruction>(instruction))
        {
            const BindVertexBufferInstruction& bind = std::get<BindVertexBufferInstruction>(instruction);
            const Ref<BufferVulkan>& buffer = bind.buffer.cast_to<BufferVulkan>();

            cb.bindVertexBuffers(bind.location, {buffer->buffer}, {0});
        }
        else if (std::holds_alternative<DispatchInstruction>(instruction))
        {
            const DispatchInstruction& dispatch = std::get<DispatchInstruction>(instruction);

            cb.dispatch(dispatch.group_x, dispatch.group_y, dispatch.group_z);
        }
    }

    // TracyVkCollect(m_tracy_context, cb);

    ERR_RESULT_E_RET(cb.end());

    vk::PipelineStageFlags wait_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::Semaphore submit_semaphore = m_submit_semaphores[image_index];

    ERR_RESULT_E_RET(m_graphics_queue.submit({vk::SubmitInfo({acquire_semaphore}, {wait_stage_mask}, {cb}, {submit_semaphore})}, m_frame_fences[m_current_frame]));
    ERR_RESULT_E_RET(m_graphics_queue.presentKHR(vk::PresentInfoKHR({submit_semaphore}, {m_swapchain}, {image_index})));

    m_current_frame = (m_current_frame + 1) % max_frames_in_flight;
}

Result<vk::Pipeline> RenderingDriverVulkan::create_graphics_pipeline(const Ref<Shader>& shader, std::optional<InstanceLayout> instance_layout, vk::PolygonMode polygon_mode, vk::CullModeFlags cull_mode, MaterialFlags flags, vk::PipelineLayout pipeline_layout, vk::RenderPass render_pass, bool previous_depth_pass)
{
    ZoneScoped;

    StackVector<vk::PipelineShaderStageCreateInfo, 2> shader_stages;

    for (ShaderStageFlagBits stage : shader->get_stage_mask().as_vector())
    {
        std::vector<uint32_t> code = shader->get_code(stage);
        auto shader_module_result = m_device.createShaderModule(vk::ShaderModuleCreateInfo({}, code.size() * sizeof(uint32_t), code.data()));
        YEET_RESULT(shader_module_result);

        shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, convert_shader_stage(stage), shader_module_result.value, "main"));
    }

    StackVector<vk::VertexInputBindingDescription, 4> input_bindings; // 4 or 3 elements depending on `instance_layout`

    std::vector<vk::VertexInputAttributeDescription> input_attribs;
    input_attribs.reserve(3 + (instance_layout.has_value() ? instance_layout->inputs.size() : 0));

    input_bindings.push_back(vk::VertexInputBindingDescription(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex)); // position
    input_bindings.push_back(vk::VertexInputBindingDescription(1, sizeof(glm::vec3), vk::VertexInputRate::eVertex)); // normal
    input_bindings.push_back(vk::VertexInputBindingDescription(2, sizeof(glm::vec2), vk::VertexInputRate::eVertex)); // uv

    input_attribs.push_back(vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0)); // position
    input_attribs.push_back(vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32G32B32Sfloat, 0)); // normal
    input_attribs.push_back(vk::VertexInputAttributeDescription(2, 2, vk::Format::eR32G32Sfloat, 0));    // uv

    if (instance_layout.has_value())
    {
        input_bindings.push_back(vk::VertexInputBindingDescription(3, instance_layout->stride, vk::VertexInputRate::eInstance));

        size_t location = 3;

        for (const auto& input : instance_layout->inputs)
        {
            input_attribs.push_back(vk::VertexInputAttributeDescription(location, 3, convert_shader_type(input.type), input.offset));
            location += 1;
        }
    }

    std::array<vk::DynamicState, 2> dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic_state_info({}, dynamic_states);

    vk::PipelineVertexInputStateCreateInfo vertex_input_info({}, input_bindings, input_attribs);
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info({}, vk::PrimitiveTopology::eTriangleList, vk::False);
    vk::PipelineViewportStateCreateInfo viewport_info({}, 1, nullptr, 1, nullptr);
    vk::PipelineRasterizationStateCreateInfo rasterization_info({}, vk::False, vk::False, polygon_mode, cull_mode, vk::FrontFace::eCounterClockwise, vk::False, 0.0, 0.0, 0.0, 1.0);
    vk::PipelineMultisampleStateCreateInfo multisample_info({}, vk::SampleCountFlagBits::e1, vk::False, 1.0, nullptr, vk::False, vk::False);

    vk::PipelineColorBlendAttachmentState color_blend_state{};

    if (!flags.transparency)
    {
        color_blend_state.blendEnable = vk::False;
        color_blend_state.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_state.dstColorBlendFactor = vk::BlendFactor::eZero;
        color_blend_state.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_state.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        color_blend_state.alphaBlendOp = vk::BlendOp::eAdd;
        color_blend_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }
    else
    {
        color_blend_state.blendEnable = vk::True;
        color_blend_state.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        color_blend_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_state.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_state.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        color_blend_state.alphaBlendOp = vk::BlendOp::eAdd;
        color_blend_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::CompareOp depth_compare_op = previous_depth_pass ? vk::CompareOp::eEqual : (flags.priority == PriorityNormal ? vk::CompareOp::eLess : vk::CompareOp::eLessOrEqual);

    vk::PipelineColorBlendStateCreateInfo blend_info({}, vk::False, vk::LogicOp::eCopy, {color_blend_state});
    vk::PipelineDepthStencilStateCreateInfo depth_info({}, vk::True, vk::True, depth_compare_op, vk::False, vk::False, {}, {}, 0.0, 1.0);

    auto pipeline_result = m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo(
                                                                        {},
                                                                        shader_stages.size(),
                                                                        shader_stages.data(),
                                                                        &vertex_input_info,
                                                                        &input_assembly_info,
                                                                        nullptr, // tesselation
                                                                        &viewport_info,
                                                                        &rasterization_info,
                                                                        &multisample_info,
                                                                        &depth_info,
                                                                        &blend_info,
                                                                        &dynamic_state_info,
                                                                        pipeline_layout,
                                                                        render_pass,
                                                                        0, // subpass
                                                                        nullptr, 0));
    YEET_RESULT(pipeline_result);

    for (const auto& shader_stage : shader_stages)
    {
        m_device.destroyShaderModule(shader_stage.module);
    }

    return pipeline_result.value;
}

Result<vk::Pipeline> RenderingDriverVulkan::create_compute_pipeline(const Ref<Shader>& shader, vk::PipelineLayout pipeline_layout)
{
    std::vector<uint32_t> code = shader->get_code(ShaderStageFlagBits::Compute);
    auto shader_module_result = m_device.createShaderModule(vk::ShaderModuleCreateInfo({}, code.size() * sizeof(uint32_t), code.data()));
    YEET_RESULT(shader_module_result);

    vk::PipelineShaderStageCreateInfo shader_info({}, vk::ShaderStageFlagBits::eCompute, shader_module_result.value, "compute_main");

    auto pipeline_result = m_device.createComputePipeline(nullptr, vk::ComputePipelineCreateInfo({}, shader_info, pipeline_layout));
    YEET_RESULT(pipeline_result);

    return pipeline_result.value;
}

Result<Ref<Texture>> RenderingDriverVulkan::create_texture_from_vk_image(vk::Image image, uint32_t width, uint32_t height, vk::Format format)
{
    vk::ImageAspectFlags aspect_mask = format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image, nullptr, image_view_result.value, width, height, 0, aspect_mask, 1, false).cast_to<Texture>();
}

std::optional<uint32_t> RenderingDriverVulkan::find_memory_type_index(uint32_t type_bits, vk::MemoryPropertyFlags properties)
{
    uint32_t bits = type_bits;

    for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++)
    {
        if (bits & 1 && (m_memory_properties.memoryTypes[(int32_t)i].propertyFlags & properties) == properties)
            return i;
        bits >>= 1;
    }

    return std::nullopt;
}

Result<vk::DeviceMemory> RenderingDriverVulkan::allocate_memory_for_buffer(vk::Buffer buffer, vk::MemoryPropertyFlags properties)
{
    vk::MemoryRequirements requirements = m_device.getBufferMemoryRequirements(buffer);
    auto memory_type_index_opt = find_memory_type_index(requirements.memoryTypeBits, properties);

    if (!memory_type_index_opt.has_value())
        return Error(ErrorKind::OutOfDeviceMemory);

    uint32_t memory_type_index = memory_type_index_opt.value();
    auto memory_result = m_device.allocateMemory(vk::MemoryAllocateInfo(requirements.size, memory_type_index));
    YEET_RESULT(memory_result);

    auto bind_result = m_device.bindBufferMemory(buffer, memory_result.value, 0);
    YEET_RESULT_E(bind_result);

    return memory_result.value;
}

Result<vk::DeviceMemory> RenderingDriverVulkan::allocate_memory_for_image(vk::Image image, vk::MemoryPropertyFlags properties)
{
    vk::MemoryRequirements requirements = m_device.getImageMemoryRequirements(image);
    auto memory_type_index_opt = find_memory_type_index(requirements.memoryTypeBits, properties);

    if (!memory_type_index_opt.has_value())
        return Error(ErrorKind::OutOfDeviceMemory);

    uint32_t memory_type_index = memory_type_index_opt.value();
    auto memory_result = m_device.allocateMemory(vk::MemoryAllocateInfo(requirements.size, memory_type_index));
    YEET_RESULT(memory_result);

    auto bind_result = m_device.bindImageMemory(image, memory_result.value, 0);
    YEET_RESULT_E(bind_result);

    return memory_result.value;
}

static bool contains_ext(const std::vector<vk::ExtensionProperties>& extensions, const char *ext)
{
    return std::find_if(extensions.begin(), extensions.end(), [ext](auto& extension)
                        { return std::strcmp(extension.extensionName, ext) == 0; }) != extensions.end();
}

static int32_t calculate_device_score(const vk::PhysicalDeviceProperties& properties, const std::vector<vk::ExtensionProperties>& extensions, const std::vector<const char *>& required_extensions, const std::vector<const char *>& optional_extensions)
{
    int32_t score = 0;

    switch (properties.deviceType)
    {
    case vk::PhysicalDeviceType::eDiscreteGpu:
        score += 100;
        break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        score += 10;
        break;
    case vk::PhysicalDeviceType::eCpu:
        score += 1;
        break;
    default:
        break;
    }

    // TODO: physical device features

    for (auto& ext : required_extensions)
    {
        if (!contains_ext(extensions, ext))
            return 0;
    }

    for (auto& ext : optional_extensions)
    {
        if (!contains_ext(extensions, ext))
            score += 20;
    }

    return score;
}

Result<QueueInfo, bool> RenderingDriverVulkan::find_queue(vk::PhysicalDevice physical_device)
{
    const std::vector<vk::QueueFamilyProperties> queue_properties = physical_device.getQueueFamilyProperties();
    QueueInfo queue_info;

    // Select a graphics queue
    for (size_t i = 0; i < queue_properties.size(); i++)
    {
        const auto& queue_property = queue_properties[i];

        if (queue_property.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queue_info.graphics_index = i;

            bool present_support = physical_device.getSurfaceSupportKHR(i, m_surface).value;

            if (!present_support)
                return false;

            break;
        }
    }

    // Select a compute queue
    for (size_t i = 0; i < queue_properties.size(); i++)
    {
        const auto& queue_property = queue_properties[i];

        if (queue_property.queueFlags & vk::QueueFlagBits::eCompute && i != queue_info.graphics_index)
        {
            queue_info.compute_index = i;
        }
    }

    return queue_info;
}

std::optional<PhysicalDeviceWithInfo> RenderingDriverVulkan::pick_best_device(const std::vector<vk::PhysicalDevice>& physical_devices, const std::vector<const char *>& required_extensions, const std::vector<const char *>& optional_extensions)
{
    std::optional<PhysicalDeviceWithInfo> best_device = std::nullopt;
    int32_t best_score = 0;

    for (const auto& physical_device : physical_devices)
    {
        vk::PhysicalDeviceProperties properties = physical_device.getProperties();
        vk::PhysicalDeviceFeatures features = physical_device.getFeatures();

        std::vector<vk::ExtensionProperties> extensions = physical_device.enumerateDeviceExtensionProperties().value;

        int32_t score = calculate_device_score(properties, extensions, required_extensions, optional_extensions);
        QueueInfo queue_info = find_queue(physical_device).value();

        // FIXME: Detect supported surface formats.
        vk::SurfaceFormatKHR surface_format(vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear);

        if (score > best_score)
        {
            best_score = score;
            best_device = PhysicalDeviceWithInfo{
                .physical_device = physical_device,
                .properties = properties,
                .features = features,
                .extensions = extensions,
                .queue_info = queue_info,
                .surface_format = surface_format,
            };
        }
    }

    return best_device;
}

BufferVulkan::~BufferVulkan()
{
    RenderingDriverVulkan::get()->get_device().freeMemory(memory);
    RenderingDriverVulkan::get()->get_device().destroyBuffer(buffer);
}

TextureVulkan::~TextureVulkan()
{
    RenderingDriverVulkan::get()->get_device().destroyImageView(image_view);

    if (owned)
    {
        RenderingDriverVulkan::get()->get_device().freeMemory(memory);
        RenderingDriverVulkan::get()->get_device().destroyImage(image);
    }
}

void TextureVulkan::update(View<uint8_t> view, uint32_t layer)
{
    ERR_COND(view.size() > size, "Out of bounds");

    if (view.size() == 0)
        return;

    auto buffer_result = RenderingDriverVulkan::get()->create_buffer(view.size(), BufferUsageFlagBits::CopySource, BufferVisibility::GPUAndCPU);
    ERR_EXPECT_R(buffer_result, "failed to create the staging buffer");

    Ref<BufferVulkan> staging_buffer_vk = buffer_result.value().cast_to<BufferVulkan>();

    // Copy the data into the staging buffer.
    auto map_result = RenderingDriverVulkan::get()->get_device().mapMemory(staging_buffer_vk->memory, 0, view.size(), {});
    ERR_RESULT_RET(map_result);
    std::memcpy(map_result.value, view.data(), view.size());
    RenderingDriverVulkan::get()->get_device().unmapMemory(staging_buffer_vk->memory);

    // Copy from the staging buffer to the final buffer
    std::lock_guard<std::mutex> lock(RenderingDriverVulkan::get()->get_graphics_mutex());
    vk::CommandBuffer cb = RenderingDriverVulkan::get()->get_transfer_buffer();

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    vk::BufferImageCopy region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, layer, 1), {}, vk::Extent3D(m_width, m_height, 1));

    cb.copyBufferToImage(staging_buffer_vk->buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
    ERR_RESULT_E_RET(cb.end());

    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().submit({vk::SubmitInfo(0, nullptr, nullptr, 1, &cb)}));
    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().waitIdle());

    // TODO: Do the same as `Buffer::update`
}

static vk::AccessFlags layout_to_access_mask(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::Undefined:
        return {};
    case TextureLayout::DepthStencilAttachment:
        return vk::AccessFlagBits::eDepthStencilAttachmentRead;
    case TextureLayout::CopyDst:
        return vk::AccessFlagBits::eTransferWrite;
    case TextureLayout::ShaderReadOnly:
        return vk::AccessFlagBits::eShaderRead;
    // TODO: DepthStencilReadOnly
    default:
        return {};
    }

    return {};
}

static vk::PipelineStageFlags layout_to_stage_mask(TextureLayout layout)
{
    switch (layout)
    {
    case TextureLayout::Undefined:
        return vk::PipelineStageFlagBits::eTopOfPipe;
    case TextureLayout::DepthStencilAttachment:
        return vk::PipelineStageFlagBits::eEarlyFragmentTests;
    case TextureLayout::CopyDst:
        return vk::PipelineStageFlagBits::eTransfer;
    case TextureLayout::ShaderReadOnly:
        return vk::PipelineStageFlagBits::eFragmentShader;
    // TODO: DepthStencilReadOnly
    default:
        return {};
    }

    return {};
}

void TextureVulkan::transition_layout(TextureLayout new_layout)
{
    std::lock_guard<std::mutex> lock(RenderingDriverVulkan::get()->get_graphics_mutex());
    vk::CommandBuffer cb = RenderingDriverVulkan::get()->get_transfer_buffer();

    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo()));

    vk::ImageMemoryBarrier barrier(
        layout_to_access_mask(m_layout), layout_to_access_mask(new_layout),
        convert_texture_layout(m_layout), convert_texture_layout(new_layout),
        0, 0,
        image,
        vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, layers));

    vk::PipelineStageFlags src_stage_mask = layout_to_stage_mask(m_layout);
    vk::PipelineStageFlags dst_stage_mask = layout_to_stage_mask(new_layout);

    cb.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, {barrier});
    ERR_RESULT_E_RET(cb.end());

    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().submit({vk::SubmitInfo({}, {}, {cb}, {})}));
    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().waitIdle());

    m_layout = new_layout;
}

Result<DescriptorPool> DescriptorPool::create(vk::DescriptorSetLayout layout, Ref<Shader> shader)
{
    uint32_t image_sampler_count = 0;
    uint32_t uniform_buffer_count = 0;

    for (const auto& binding : shader->get_bindings())
    {
        switch (binding.second.kind)
        {
        case BindingKind::Texture:
            image_sampler_count += 1;
            break;
        case BindingKind::UniformBuffer:
            uniform_buffer_count += 1;
            break;
        }
    }

    std::vector<vk::DescriptorPoolSize> sizes;
    if (image_sampler_count > 0)
    {
        sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, image_sampler_count));
    }
    if (uniform_buffer_count > 0)
    {
        sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, uniform_buffer_count));
    }

    auto pool_result = RenderingDriverVulkan::get()->get_device().createDescriptorPool(vk::DescriptorPoolCreateInfo({}, max_sets, sizes.size(), sizes.data()));
    YEET_RESULT(pool_result);

    return DescriptorPool(layout, std::move(sizes));
}

DescriptorPool::~DescriptorPool()
{
}

Result<vk::DescriptorSet> DescriptorPool::allocate()
{
    if (m_allocation_count / max_sets >= m_pools.size())
    {
        YEET(add_pool());
    }

    vk::DescriptorPool pool = m_pools[m_allocation_count / max_sets];

    auto descriptor_set_result = RenderingDriverVulkan::get()->get_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo(pool, 1, &m_layout));
    YEET_RESULT(descriptor_set_result);

    m_allocation_count += 1;

    return descriptor_set_result.value[0];
}

Result<> DescriptorPool::add_pool()
{
    auto pool_result = RenderingDriverVulkan::get()->get_device().createDescriptorPool(vk::DescriptorPoolCreateInfo({}, max_sets, m_sizes.size(), m_sizes.data()));
    YEET_RESULT(pool_result);

    m_pools.push_back(pool_result.value);

    return 0;
}

MaterialLayoutVulkan::~MaterialLayoutVulkan()
{
    RenderingDriverVulkan::get()->get_device().destroyPipelineLayout(m_pipeline_layout);
    RenderingDriverVulkan::get()->get_device().destroyDescriptorSetLayout(m_descriptor_set_layout);
}

void MaterialVulkan::set_param(const std::string& name, const Ref<Texture>& texture)
{
    Ref<MaterialLayoutVulkan> layout_vk = m_layout.cast_to<MaterialLayoutVulkan>();

    std::optional<Binding> binding_result = layout_vk->m_shader->get_binding(name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name);

    Binding binding = binding_result.value();

    Ref<TextureVulkan> texture_vk = texture.cast_to<TextureVulkan>();

    SamplerDescriptor sampler_desc = layout_vk->m_shader->get_sampler(name);

    auto sampler_result = RenderingDriverVulkan::get()->get_sampler_cache().get_or_create(sampler_desc);
    ERR_COND_V(!sampler_result.has_value(), "Failed to create sampler for parameter `%s`", name.c_str());

    vk::DescriptorImageInfo image_info(sampler_result.value(), texture_vk->image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write_image(descriptor_set, binding.binding, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info, nullptr, nullptr);

    RenderingDriverVulkan::get()->get_device().updateDescriptorSets({write_image}, {});
}

void MaterialVulkan::set_param(const std::string& name, const Ref<Buffer>& buffer)
{
    Ref<MaterialLayoutVulkan> layout_vk = m_layout.cast_to<MaterialLayoutVulkan>();

    std::optional<Binding> binding_result = layout_vk->m_shader->get_binding(name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name);

    Binding binding = binding_result.value();

    Ref<BufferVulkan> buffer_vk = buffer.cast_to<BufferVulkan>();

    vk::DescriptorBufferInfo buffer_info(buffer_vk->buffer, 0, buffer_vk->size());
    vk::WriteDescriptorSet write_buffer(descriptor_set, binding.binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info, nullptr);

    RenderingDriverVulkan::get()->get_device().updateDescriptorSets({write_buffer}, {});
}
