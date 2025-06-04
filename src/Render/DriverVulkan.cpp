#include "Render/DriverVulkan.hpp"
#include "Core/StackVector.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <chrono>
#include <fstream>
#include <print>

#include <SDL3/SDL_vulkan.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

static inline vk::BufferUsageFlags convert_buffer_usage(BufferUsage usage)
{
    vk::BufferUsageFlags flags{};

    if (usage.copy_src)
        flags |= vk::BufferUsageFlagBits::eTransferSrc;
    if (usage.copy_dst)
        flags |= vk::BufferUsageFlagBits::eTransferDst;
    if (usage.uniform)
        flags |= vk::BufferUsageFlagBits::eUniformBuffer;
    if (usage.index)
        flags |= vk::BufferUsageFlagBits::eIndexBuffer;
    if (usage.vertex)
        flags |= vk::BufferUsageFlagBits::eVertexBuffer;

    return flags;
}

static inline vk::ImageUsageFlags convert_texture_usage(TextureUsage usage)
{
    vk::ImageUsageFlags flags{};

    if (usage.copy_src)
        flags |= vk::ImageUsageFlagBits::eTransferSrc;
    if (usage.copy_dst)
        flags |= vk::ImageUsageFlagBits::eTransferDst;
    if (usage.sampled)
        flags |= vk::ImageUsageFlagBits::eSampled;
    if (usage.color_attachment)
        flags |= vk::ImageUsageFlagBits::eColorAttachment;
    if (usage.depth_attachment)
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
    case TextureFormat::D32:
        return vk::Format::eD32Sfloat;
    }

    return vk::Format::eUndefined;
}

static inline vk::Format convert_shader_type(ShaderType type)
{
    switch (type)
    {
    case ShaderType::Float:
        return vk::Format::eR32Sfloat;
    case ShaderType::Vec2:
        return vk::Format::eR32G32Sfloat;
    case ShaderType::Vec3:
        return vk::Format::eR32G32B32Sfloat;
    case ShaderType::Vec4:
        return vk::Format::eR32G32B32A32Sfloat;
    case ShaderType::Uint:
        return vk::Format::eR32Uint;
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

static vk::ShaderStageFlagBits convert_shader_stage(ShaderKind kind)
{
    switch (kind)
    {
    case ShaderKind::Vertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderKind::Fragment:
        return vk::ShaderStageFlagBits::eFragment;
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

static vk::SamplerCreateInfo convert_sampler(Sampler sampler)
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

PipelineCache::PipelineCache()
{
}

Expected<vk::Pipeline> PipelineCache::get_or_create(Material *material, vk::RenderPass render_pass)
{
    auto iter = m_pipelines.find({.material = material, .render_pass = render_pass});

    if (iter != m_pipelines.end())
    {
        return iter->second;
    }
    else
    {
        MaterialVulkan *material_vk = (MaterialVulkan *)material;
        Ref<MaterialLayoutVulkan> layout = material_vk->get_layout().cast_to<MaterialLayoutVulkan>();

        auto pipeline_result = RenderingDriverVulkan::get()->create_graphics_pipeline(layout->m_shader, layout->m_instance_layout, layout->m_polygon_mode, layout->m_cull_mode, layout->m_flags, layout->m_pipeline_layout, render_pass);
        YEET(pipeline_result);

        m_pipelines[{.material = material, .render_pass = render_pass}] = pipeline_result.value();
        return pipeline_result.value();
    }
}

SamplerCache::SamplerCache()
{
}

Expected<vk::Sampler> SamplerCache::get_or_create(Sampler sampler)
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

RenderingDriverVulkan::RenderingDriverVulkan()
{
}

RenderingDriverVulkan::~RenderingDriverVulkan()
{
    if (m_device)
    {
        destroy_swapchain();

        m_device.destroyRenderPass(m_render_pass);

        m_device.destroyQueryPool(m_timestamp_query_pool);

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            m_device.destroySemaphore(m_acquire_semaphores[i]);
            m_device.destroySemaphore(m_submit_semaphores[i]);
            m_device.destroyFence(m_frame_fences[i]);
        }

        m_device.freeCommandBuffers(m_graphics_command_pool, m_command_buffers);
        m_device.destroyCommandPool(m_graphics_command_pool);

        m_device.destroy();
    }

    if (m_instance)
    {
        m_instance.destroySurfaceKHR(m_surface);
        m_instance.destroy();
    }
}

std::expected<void, Error> RenderingDriverVulkan::initialize(const Window& window)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    Uint32 instance_extensions_count = 0;
    auto instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extensions_count);

    vk::ApplicationInfo app_info("ft_minecraft", 0, "No engine", 0, VK_API_VERSION_1_2);

    std::vector<const char *> validation_layers;

#ifdef __DEBUG__
    validation_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

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
    if (instance_result.result != vk::Result::eSuccess)
        return std::unexpected(instance_result.result);
    m_instance = instance_result.value;

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

    VkSurfaceKHR surface;

    if (!SDL_Vulkan_CreateSurface(window.get_window_ptr(), m_instance, nullptr, &surface))
        return std::unexpected(ErrorKind::BadDriver);
    m_surface = surface;

    // Select the best physical device
    // vk::PhysicalDeviceFeatures required_features = {};
    // vk::PhysicalDeviceFeatures optional_features = {};

    std::vector<const char *> required_extensions;
    std::vector<const char *> optional_extensions;

    required_extensions.push_back("VK_KHR_swapchain");
    required_extensions.push_back("VK_EXT_host_query_reset");

#ifdef TRACY_ENABLE
    // This is widely supported except for android. This is only needed when profiling with tracy anyway.
    required_extensions.push_back("VK_EXT_calibrated_timestamps");
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
        return std::unexpected(ErrorKind::NoSuitableDevice);

    m_physical_device = physical_device_with_info_result->physical_device;
    m_physical_device_properties = physical_device_with_info_result->properties;
    m_surface_format = physical_device_with_info_result->surface_format;

    std::println("info: GPU selected: {}", m_physical_device_properties.deviceName.data());

    auto surface_capabilities_result = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    if (surface_capabilities_result.result != vk::Result::eSuccess)
        return std::unexpected(surface_capabilities_result.result);
    m_surface_capabilities = surface_capabilities_result.value;

    auto surface_present_modes_result = m_physical_device.getSurfacePresentModesKHR(m_surface);
    if (surface_present_modes_result.result != vk::Result::eSuccess)
        return std::unexpected(surface_present_modes_result.result);
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

    // Create a render pass for the output
    std::array<vk::AttachmentDescription, 2> attachments{
        vk::AttachmentDescription(
            {},
            m_surface_format.format, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR),
        vk::AttachmentDescription(
            {},
            convert_texture_format(TextureFormat::D32), vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal),
    };

    const vk::AttachmentReference color_attach(0, vk::ImageLayout::eColorAttachmentOptimal);
    const vk::AttachmentReference depth_attach(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, {color_attach}, {}, &depth_attach);

    vk::SubpassDependency dependency(
        vk::SubpassExternal, 0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        {}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    auto render_pass_result = m_device.createRenderPass(vk::RenderPassCreateInfo({}, attachments, {subpass}, {dependency}));
    YEET_RESULT(render_pass_result);
    m_render_pass = render_pass_result.value;

    YEET(configure_surface(window, VSync::Off));

    // This uses `VK_EXT_host_query_reset` & `VK_EXT_calibrated_timestamps`
    m_tracy_context = TracyVkContextHostCalibrated(m_instance, m_physical_device, m_device, vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr, vk::detail::defaultDispatchLoaderDynamic.vkGetDeviceProcAddr);

    m_start_time = std::chrono::high_resolution_clock::now();

    return {};
}

Expected<void> RenderingDriverVulkan::configure_surface(const Window& window, VSync vsync)
{
    (void)m_device.waitIdle();

    vk::PresentModeKHR present_mode;

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

    auto depth_texture_result = create_texture(surface_extent.width, surface_extent.height, TextureFormat::D32, {.depth_attachment = 1});
    YEET(depth_texture_result);

    Ref<TextureVulkan> depth_texture_vk = depth_texture_result.value().cast_to<TextureVulkan>();

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

        Ref<TextureVulkan> texture = texture_result.value().cast_to<TextureVulkan>();

        swapchain_textures.push_back(texture.cast_to<Texture>());

        std::array<vk::ImageView, 2> attachments = {texture->image_view, depth_texture_vk->image_view};

        auto framebuffer_result = m_device.createFramebuffer(vk::FramebufferCreateInfo({}, m_render_pass, attachments, surface_extent.width, surface_extent.height, 1));
        YEET_RESULT(framebuffer_result);

        swapchain_framebuffers.push_back(framebuffer_result.value);
    }

    destroy_swapchain();

    m_swapchain = swapchain_result.value;
    m_depth_texture = depth_texture_result.value();
    m_swapchain_images = std::move(swapchain_images_result.value);
    m_swapchain_textures = std::move(swapchain_textures);
    m_swapchain_framebuffers = std::move(swapchain_framebuffers);
    m_surface_extent = Extent2D(surface_extent.width, surface_extent.height);

    return {};
}

void RenderingDriverVulkan::destroy_swapchain()
{
    if (m_swapchain)
    {
        for (const auto& fb : m_swapchain_framebuffers)
            m_device.destroyFramebuffer(fb);

        m_swapchain_textures.clear();

        m_device.destroySwapchainKHR(m_swapchain);
    }
}

void RenderingDriverVulkan::limit_frames(uint32_t limit)
{
    m_frames_limit = limit;
    m_time_between_frames = 1'000'000 / m_frames_limit;
}

Expected<Ref<Buffer>> RenderingDriverVulkan::create_buffer(size_t size, BufferUsage usage, BufferVisibility visibility)
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

Expected<Ref<Texture>> RenderingDriverVulkan::create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
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
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::D32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::e2D, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, 1, true).cast_to<Texture>();
}

Expected<Ref<Texture>> RenderingDriverVulkan::create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers)
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
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::D32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::e2DArray, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, layers, true).cast_to<Texture>();
}

Expected<Ref<Texture>> RenderingDriverVulkan::create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
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
    vk::ImageAspectFlags aspect_mask = format == TextureFormat::D32 ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image_result.value, vk::ImageViewType::eCube, vk_format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image_result.value, memory_result.value(), image_view_result.value, width, height, width * height * size_of(format), aspect_mask, 1, true).cast_to<Texture>();
}

Expected<Ref<Mesh>> RenderingDriverVulkan::create_mesh(IndexType index_type, Span<uint8_t> indices, Span<glm::vec3> vertices, Span<glm::vec2> uvs, Span<glm::vec3> normals)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    auto index_buffer_result = create_buffer(indices.size(), {.copy_dst = 1, .index = 1});
    YEET(index_buffer_result);
    Ref<Buffer> index_buffer = index_buffer_result.value();

    auto vertex_buffer_result = create_buffer(vertices.size() * sizeof(glm::vec3), {.copy_dst = 1, .vertex = 1});
    YEET(vertex_buffer_result);
    Ref<Buffer> vertex_buffer = vertex_buffer_result.value();

    auto normal_buffer_result = create_buffer(normals.size() * sizeof(glm::vec3), {.copy_dst = 1, .vertex = 1});
    YEET(normal_buffer_result);
    Ref<Buffer> normal_buffer = normal_buffer_result.value();

    auto uv_buffer_result = create_buffer(uvs.size() * sizeof(glm::vec2), {.copy_dst = 1, .vertex = 1});
    YEET(uv_buffer_result);
    Ref<Buffer> uv_buffer = uv_buffer_result.value();

    index_buffer->update(indices.as_bytes());
    vertex_buffer->update(vertices.as_bytes());
    normal_buffer->update(normals.as_bytes());
    uv_buffer->update(uvs.as_bytes());

    return make_ref<MeshVulkan>(index_type, convert_index_type(index_type), vertex_count, index_buffer, vertex_buffer, normal_buffer, uv_buffer).cast_to<Mesh>();
}

Expected<Ref<MaterialLayout>> RenderingDriverVulkan::create_material_layout(Shader shader, Span<MaterialParam> params, MaterialFlags flags, std::optional<InstanceLayout> instance_layout, CullMode cull_mode, PolygonMode polygon_mode)
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(params.size());

    uint32_t binding = 0;

    for (const auto& param : params)
    {
        switch (param.kind)
        {
        case MaterialParamKind::Texture:
        {
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding++, vk::DescriptorType::eSampledImage, 1, convert_shader_stage(param.shader_kind), nullptr));
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding++, vk::DescriptorType::eSampler, 1, convert_shader_stage(param.shader_kind), nullptr));
        }
        break;
        case MaterialParamKind::UniformBuffer:
        {
            bindings.push_back(vk::DescriptorSetLayoutBinding(binding++, vk::DescriptorType::eUniformBuffer, 1, convert_shader_stage(param.shader_kind), nullptr));
        }
        break;
        }
    }

    auto layout_result = RenderingDriverVulkan::get()->get_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo({}, bindings));
    YEET_RESULT(layout_result);

    auto pool_result = DescriptorPool::create(layout_result.value, params);
    YEET(pool_result);

    std::array<vk::PushConstantRange, 1> push_constant_ranges{
        vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants)),
    };
    std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts{layout_result.value};

    auto pipeline_layout_result = RenderingDriverVulkan::get()->get_device().createPipelineLayout(vk::PipelineLayoutCreateInfo({}, descriptor_set_layouts, push_constant_ranges));
    YEET_RESULT(pipeline_layout_result);

    return make_ref<MaterialLayoutVulkan>(layout_result.value, pool_result.value(), shader, instance_layout, params.to_vector(), convert_polygon_mode(polygon_mode), convert_cull_mode(cull_mode), flags, pipeline_layout_result.value).cast_to<MaterialLayout>();
}

Expected<Ref<Material>> RenderingDriverVulkan::create_material(MaterialLayout *layout)
{
    MaterialLayoutVulkan *layout_vk = (MaterialLayoutVulkan *)layout;

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
        break;
    default:
        return;
    }

    ZoneScoped;

    uint32_t image_index = image_index_result.value;
    vk::Framebuffer fb = m_swapchain_framebuffers[image_index];

    vk::CommandBuffer cb = m_command_buffers[m_current_frame];

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    // TracyVkZone(m_tracy_context, cb, "DrawMesh");

    // TODO: Add synchronization

    for (const auto& instruction : graph.get_instructions())
    {
        ZoneNamed(draw_graph_instruction, true);

        switch (instruction.kind)
        {
        case InstructionKind::BeginRenderPass:
        {
            std::array<vk::ClearValue, 2> clear_values{
                vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
                vk::ClearDepthStencilValue(1.0),
            };

            cb.beginRenderPass(vk::RenderPassBeginInfo(m_render_pass, fb, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(m_surface_extent.width, m_surface_extent.height)), clear_values), vk::SubpassContents::eInline);
            break;
        }
        case InstructionKind::EndRenderPass:
        {
            cb.endRenderPass();
            break;
        }
        case InstructionKind::Draw:
        {
            MeshVulkan *mesh = (MeshVulkan *)instruction.draw.mesh;

            MaterialVulkan *material = (MaterialVulkan *)instruction.draw.material;
            const Ref<MaterialLayoutVulkan>& material_layout = material->get_layout().cast_to<MaterialLayoutVulkan>();

            std::optional<Buffer *> instance_buffer = instruction.draw.instance_buffer;

            auto pipeline_result = m_pipeline_cache.get_or_create(material, m_render_pass);
            ERR_EXPECT_B(pipeline_result, "Failed to create a pipeline for the material");

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
                cb.bindVertexBuffers(3, {((BufferVulkan *)instance_buffer.value())->buffer}, {0});
            }

            cb.setViewport(0, {vk::Viewport(0.0, 0.0, (float)m_surface_extent.width, (float)m_surface_extent.height, 0.0, 1.0)});
            cb.setScissor(0, {vk::Rect2D({0, 0}, {m_surface_extent.width, m_surface_extent.height})});

            PushConstants push_constants{
                .view_matrix = instruction.draw.view_matrix,
            };

            cb.pushConstants(material_layout->m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constants);

            cb.drawIndexed(mesh->vertex_count(), instruction.draw.instance_count, 0, 0, 0);

            break;
        }
        case InstructionKind::Copy:
        {
            // TODO
            break;
        }
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

static std::vector<uint32_t> read_shader_code(std::string filename)
{
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
    ERR_COND_V(!ifs.is_open(), "Shader {} does not exists", filename);

    if (!ifs)
    {
        return {};
    }

    size_t size = ifs.tellg();

    ifs.seekg(0, std::ios::beg);

    std::vector<uint32_t> data(size);

    ifs.read((char *)data.data(), (ssize_t)size);

    return data;
}

Expected<vk::Pipeline> RenderingDriverVulkan::create_graphics_pipeline(Shader shader, std::optional<InstanceLayout> instance_layout, vk::PolygonMode polygon_mode, vk::CullModeFlags cull_mode, MaterialFlags flags, vk::PipelineLayout pipeline_layout, vk::RenderPass render_pass)
{
    StackVector<vk::PipelineShaderStageCreateInfo, 4> shader_stages;
    const Shader::Ref& shader_ref = shader.get_ref();

    std::vector<uint32_t> code = read_shader_code(shader_ref.filename);
    auto shader_module_result = m_device.createShaderModule(vk::ShaderModuleCreateInfo({}, code.size(), code.data()));
    YEET_RESULT(shader_module_result);

    for (const auto& stage : shader_ref.stages)
    {
        shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, convert_shader_stage(stage.kind), shader_module_result.value, stage.entry.c_str()));
    }

    std::vector<vk::VertexInputBindingDescription> input_bindings;
    input_bindings.reserve(instance_layout.has_value() ? 4 : 3);

    std::vector<vk::VertexInputAttributeDescription> input_attribs;
    input_attribs.reserve(3 + (instance_layout.has_value() ? instance_layout->inputs.size() : 0));

    input_bindings.push_back(vk::VertexInputBindingDescription(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex));
    input_bindings.push_back(vk::VertexInputBindingDescription(1, sizeof(glm::vec3), vk::VertexInputRate::eVertex));
    input_bindings.push_back(vk::VertexInputBindingDescription(2, sizeof(glm::vec2), vk::VertexInputRate::eVertex));

    input_attribs.push_back(vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0));
    input_attribs.push_back(vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32G32B32Sfloat, 0));
    input_attribs.push_back(vk::VertexInputAttributeDescription(2, 2, vk::Format::eR32G32Sfloat, 0));

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

    vk::PipelineColorBlendStateCreateInfo blend_info({}, vk::False, vk::LogicOp::eCopy, {color_blend_state});
    vk::PipelineDepthStencilStateCreateInfo depth_info({}, vk::True, vk::True, flags.always_first ? vk::CompareOp::eLessOrEqual : vk::CompareOp::eLess, vk::False, vk::False, {}, {}, 0.0, 1.0);

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

    m_device.destroyShaderModule(shader_module_result.value);

    return pipeline_result.value;
}

Expected<Ref<Texture>> RenderingDriverVulkan::create_texture_from_vk_image(vk::Image image, uint32_t width, uint32_t height, vk::Format format)
{
    vk::ImageAspectFlags aspect_mask = format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    auto image_view_result = m_device.createImageView(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, {}, vk::ImageSubresourceRange(aspect_mask, 0, 1, 0, 1)));
    YEET_RESULT(image_view_result);

    return make_ref<TextureVulkan>(image, nullptr, image_view_result.value, width, height, 0, aspect_mask, 1, false).cast_to<Texture>();
}

std::optional<uint32_t> RenderingDriverVulkan::find_memory_type_index(uint32_t type_bits, vk::MemoryPropertyFlags properties)
{
    uint32_t bits = type_bits;

    for (size_t i = 0; i < m_memory_properties.memoryTypeCount; i++)
    {
        if (bits & 1 && (m_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
        bits >>= 1;
    }

    return std::nullopt;
}

std::expected<vk::DeviceMemory, Error> RenderingDriverVulkan::allocate_memory_for_buffer(vk::Buffer buffer, vk::MemoryPropertyFlags properties)
{
    vk::MemoryRequirements requirements = m_device.getBufferMemoryRequirements(buffer);
    auto memory_type_index_opt = find_memory_type_index(requirements.memoryTypeBits, properties);

    if (!memory_type_index_opt.has_value())
        return Error::unexpected<vk::DeviceMemory>(ErrorKind::OutOfDeviceMemory);

    uint32_t memory_type_index = memory_type_index_opt.value();
    auto memory_result = m_device.allocateMemory(vk::MemoryAllocateInfo(requirements.size, memory_type_index));
    YEET_RESULT(memory_result);

    auto bind_result = m_device.bindBufferMemory(buffer, memory_result.value, 0);
    YEET_RESULT_E(bind_result);

    return memory_result.value;
}

std::expected<vk::DeviceMemory, Error> RenderingDriverVulkan::allocate_memory_for_image(vk::Image image, vk::MemoryPropertyFlags properties)
{
    vk::MemoryRequirements requirements = m_device.getImageMemoryRequirements(image);
    auto memory_type_index_opt = find_memory_type_index(requirements.memoryTypeBits, properties);

    if (!memory_type_index_opt.has_value())
        return Error::unexpected<vk::DeviceMemory>(ErrorKind::OutOfDeviceMemory);

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

std::expected<QueueInfo, bool> RenderingDriverVulkan::find_queue(vk::PhysicalDevice physical_device)
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
                return std::unexpected(false);

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

void BufferVulkan::update(Span<uint8_t> view, size_t offset)
{
    ERR_COND_VR(view.size() > m_size - offset, "too big: {} but size is {}", view.size(), m_size - offset);

    if (view.size() == 0)
        return;

    auto buffer_result = RenderingDriverVulkan::get()->create_buffer(view.size(), {.copy_src = true}, BufferVisibility::GPUAndCPU);
    ERR_EXPECT_R(buffer_result, "failed to create the staging buffer");

    Ref<BufferVulkan> staging_buffer_vk = buffer_result.value().cast_to<BufferVulkan>();

    // Copy the data into the staging buffer.
    auto map_result = RenderingDriverVulkan::get()->get_device().mapMemory(staging_buffer_vk->memory, 0, view.size(), {});
    ERR_RESULT_RET(map_result);
    std::memcpy(map_result.value, view.data(), view.size());
    RenderingDriverVulkan::get()->get_device().unmapMemory(staging_buffer_vk->memory);

    // Copy from the staging buffer to the final buffer
    vk::CommandBuffer cb = RenderingDriverVulkan::get()->get_transfer_buffer();

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    vk::BufferCopy region(0, offset, std::min(view.size(), m_size - offset));

    cb.copyBuffer(staging_buffer_vk->buffer, buffer, {region});
    ERR_RESULT_E_RET(cb.end());

    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().submit({vk::SubmitInfo(0, nullptr, nullptr, 1, &cb)}));
    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().waitIdle());
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

void TextureVulkan::update(Span<uint8_t> view, uint32_t layer)
{
    ERR_COND(view.size() > size, "Out of bounds");

    if (view.size() == 0)
        return;

    auto buffer_result = RenderingDriverVulkan::get()->create_buffer(view.size(), {.copy_src = true}, BufferVisibility::GPUAndCPU);
    ERR_EXPECT_R(buffer_result, "failed to create the staging buffer");

    Ref<BufferVulkan> staging_buffer_vk = buffer_result.value().cast_to<BufferVulkan>();

    // Copy the data into the staging buffer.
    auto map_result = RenderingDriverVulkan::get()->get_device().mapMemory(staging_buffer_vk->memory, 0, view.size(), {});
    ERR_RESULT_RET(map_result);
    std::memcpy(map_result.value, view.data(), view.size());
    RenderingDriverVulkan::get()->get_device().unmapMemory(staging_buffer_vk->memory);

    // Copy from the staging buffer to the final buffer
    vk::CommandBuffer cb = RenderingDriverVulkan::get()->get_transfer_buffer();

    ERR_RESULT_E_RET(cb.reset());
    ERR_RESULT_E_RET(cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)));

    vk::BufferImageCopy region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, layer, 1), {}, vk::Extent3D(m_width, m_height, 1));

    cb.copyBufferToImage(staging_buffer_vk->buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
    ERR_RESULT_E_RET(cb.end());

    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().submit({vk::SubmitInfo(0, nullptr, nullptr, 1, &cb)}));
    ERR_RESULT_E_RET(RenderingDriverVulkan::get()->get_graphics_queue().waitIdle());
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

Expected<DescriptorPool> DescriptorPool::create(vk::DescriptorSetLayout layout, Span<MaterialParam> params)
{
    uint32_t image_sampler_count = 0;
    uint32_t uniform_buffer_count = 0;

    for (const auto& param : params)
    {
        switch (param.kind)
        {
        case MaterialParamKind::Texture:
            image_sampler_count += 1;
            break;
        case MaterialParamKind::UniformBuffer:
            uniform_buffer_count += 1;
            break;
        }
    }

    std::vector<vk::DescriptorPoolSize> sizes;
    if (image_sampler_count > 0)
    {
        sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, image_sampler_count));
        sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eSampler, image_sampler_count));
    }
    if (uniform_buffer_count > 0)
        sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, uniform_buffer_count));

    auto pool_result = RenderingDriverVulkan::get()->get_device().createDescriptorPool(vk::DescriptorPoolCreateInfo({}, max_sets, sizes.size(), sizes.data()));
    YEET_RESULT(pool_result);

    return DescriptorPool(layout, std::move(sizes));
}

Expected<vk::DescriptorSet> DescriptorPool::allocate()
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

Expected<void> DescriptorPool::add_pool()
{
    auto pool_result = RenderingDriverVulkan::get()->get_device().createDescriptorPool(vk::DescriptorPoolCreateInfo({}, max_sets, m_sizes.size(), m_sizes.data()));
    YEET_RESULT(pool_result);

    m_pools.push_back(pool_result.value);

    return {};
}

std::optional<uint32_t> MaterialLayoutVulkan::get_param_binding(const std::string& name)
{
    uint32_t binding = 0;

    for (const auto& param : m_params)
    {
        if (!std::strcmp(name.c_str(), param.name))
            return binding;

        binding += param.kind == MaterialParamKind::Texture ? 2 : 1; // sampler + texture
    }

    return std::nullopt;
}

std::optional<MaterialParam> MaterialLayoutVulkan::get_param(const std::string& name)
{
    for (const auto& param : m_params)
    {
        if (!std::strcmp(name.c_str(), param.name))
            return param;
    }

    return std::nullopt;
}

void MaterialVulkan::set_param(const std::string& name, const Ref<Texture>& texture)
{
    Ref<MaterialLayoutVulkan> layout_vk = m_layout.cast_to<MaterialLayoutVulkan>();

    std::optional<uint32_t> binding_result = layout_vk->get_param_binding(name);
    std::optional<MaterialParam> param = layout_vk->get_param(name);
    ERR_COND_V(!binding_result.has_value() || !param.has_value(), "Invalid parameter name `%s`", name.c_str());

    Ref<TextureVulkan> texture_vk = texture.cast_to<TextureVulkan>();

    auto sampler_result = RenderingDriverVulkan::get()->get_sampler_cache().get_or_create(param->image_opts.sampler);
    ERR_COND_V(!sampler_result.has_value(), "Failed to create sampler for parameter `%s`", name.c_str());

    vk::DescriptorImageInfo image_info(nullptr, texture_vk->image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write_image(descriptor_set, binding_result.value(), 0, 1, vk::DescriptorType::eSampledImage, &image_info, nullptr, nullptr);

    vk::DescriptorImageInfo sampler_info(sampler_result.value(), nullptr, vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet write_sampler(descriptor_set, binding_result.value() + 1, 0, 1, vk::DescriptorType::eSampler, &sampler_info, nullptr, nullptr);

    RenderingDriverVulkan::get()->get_device().updateDescriptorSets({write_image, write_sampler}, {});
}

void MaterialVulkan::set_param(const std::string& name, const Ref<Buffer>& buffer)
{
    Ref<MaterialLayoutVulkan> layout_vk = m_layout.cast_to<MaterialLayoutVulkan>();

    std::optional<uint32_t> binding_result = layout_vk->get_param_binding(name);
    ERR_COND_V(!binding_result.has_value(), "Invalid parameter name `%s`", name.c_str());

    Ref<BufferVulkan> buffer_vk = buffer.cast_to<BufferVulkan>();

    vk::DescriptorBufferInfo buffer_info(buffer_vk->buffer, 0, buffer_vk->size());
    vk::WriteDescriptorSet write_buffer(descriptor_set, binding_result.value(), 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info, nullptr);

    RenderingDriverVulkan::get()->get_device().updateDescriptorSets({write_buffer}, {});
}
