#pragma once

#include "Core/Containers/StackVector.hpp"
#include "Render/Driver.hpp"

#include <chrono>
#include <map>

using gpu_time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

class TextureVulkan;

struct QueueInfo
{
    std::optional<uint32_t> graphics_index;
    std::optional<uint32_t> compute_index;
};

struct PhysicalDeviceWithInfo
{
    vk::PhysicalDevice physical_device;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    std::vector<vk::ExtensionProperties> extensions;

    QueueInfo queue_info;

    vk::SurfaceFormatKHR surface_format;
};

class PipelineCache
{
public:
    struct Key
    {
        Ref<Material> material;
        vk::RenderPass render_pass;
        bool depth_pass;
        bool use_previous_depth_pass;

        bool operator<(const Key& key) const
        {
            return material.ptr() < key.material.ptr() || render_pass < key.render_pass || depth_pass < key.depth_pass || use_previous_depth_pass < key.use_previous_depth_pass;
        }
    };

    Result<vk::Pipeline> get_or_create(Ref<Material> material, vk::RenderPass render_pass, bool depth_pass, bool use_previous_depth_pass);

private:
    std::map<Key, vk::Pipeline> m_pipelines;
};

class SamplerCache
{
public:
    Result<vk::Sampler> get_or_create(SamplerDescriptor sampler);

private:
    std::map<SamplerDescriptor, vk::Sampler> m_samplers;
};

class RenderPassCache
{
public:
    Result<vk::RenderPass> get_or_create(RenderPassDescriptor descriptor);

private:
    std::map<RenderPassDescriptor, vk::RenderPass> m_render_passes;
};

class FramebufferCache
{
public:
    struct Key
    {
        std::vector<vk::ImageView> views;
        vk::RenderPass render_pass;
        uint32_t width;
        uint32_t height;

        bool operator<(const Key& other) const
        {
            return std::tie(views, render_pass, width, height) < std::tie(other.views, render_pass, width, height);
        }
    };

    Result<vk::Framebuffer> get_or_create(Key key);

    /**
     * @brief Remove every framebuffer with the specified size.
     */
    void clear_with_size(uint32_t width, uint32_t height);

    /**
     * @brief Remove every framebuffer with a specific render pass.
     */
    void clear_with_renderpass(vk::RenderPass render_pass);

private:
    std::map<Key, vk::Framebuffer> m_framebuffers;
};

class RenderGraphCache
{
public:
    struct Attachment
    {
        Ref<TextureVulkan> texture = nullptr;
        bool surface_texture = false;
        bool depth_load_previous = false;
    };

    struct RenderPass
    {
        std::string name = "";
        vk::RenderPass render_pass = nullptr;
        std::vector<Attachment> attachments = {};
        std::optional<Attachment> depth_attachment = std::nullopt;
    };

    RenderGraphCache::RenderPass& set_render_pass(uint32_t index, RenderPassDescriptor desc);
    RenderGraphCache::RenderPass& get_render_pass(int32_t index);

private:
    std::vector<RenderPass> m_render_passes;
};

class StagingBufferPool
{
public:
    struct Key
    {
        size_t size = 0;
        BufferUsage usage = {};
        BufferVisibility visibility = BufferVisibility::GPUOnly;

        bool operator<(const Key& other) const
        {
            return std::tie(size, usage, visibility) < std::tie(other.size, other.usage, other.visibility);
        }
    };

    /**
     * @brief Try find an unused buffer with the correct parameter or create it.
     */
    Result<Ref<Buffer>> get_or_create(size_t size, BufferUsage usage, BufferVisibility visibility);

private:
    std::map<Key, Ref<Buffer>> m_buffers;
};

class RenderingDriverVulkan final : public RenderingDriver
{
    CLASS(RenderingDriverVulkan, RenderingDriver);

public:
    RenderingDriverVulkan();
    virtual ~RenderingDriverVulkan() override;

    static RenderingDriverVulkan *get()
    {
        return (RenderingDriverVulkan *)RenderingDriver::get();
    }

    [[nodiscard]]
    virtual Result<> initialize(const Window& window, bool enable_validation) override;

    [[nodiscard]]
    Result<> initialize_imgui() override;

    [[nodiscard]]
    virtual Result<> configure_surface(const Window& window, VSync vsync) override;

    virtual void poll() override;

    virtual void limit_frames(uint32_t limit) override;

    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer(size_t size, BufferUsage usage = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) override;

    void update_buffer(const Ref<Buffer>& dest, View<uint8_t> view, size_t offset) override;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) override;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers) override;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) override;

    [[nodiscard]]
    virtual Result<Ref<Mesh>> create_mesh(IndexType index_type, View<uint8_t> indices, View<glm::vec3> vertices, View<glm::vec2> uvs, View<glm::vec3> normals) override;

    [[nodiscard]]
    virtual Result<Ref<MaterialLayout>> create_material_layout(Ref<Shader> shader, MaterialFlags flags = {}, std::optional<InstanceLayout> instance_layout = std::nullopt, CullMode cull_mode = CullMode::Back, PolygonMode polygon_mode = PolygonMode::Fill) override;

    [[nodiscard]]
    virtual Result<Ref<Material>> create_material(const Ref<MaterialLayout>& layout) override;

    virtual void draw_graph(const RenderGraph& graph) override;

    Result<vk::Pipeline> create_graphics_pipeline(Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, vk::PolygonMode polygon_mode, vk::CullModeFlags cull_mode, MaterialFlags flags, vk::PipelineLayout pipeline_layout, vk::RenderPass render_pass, bool previous_depth_pass);

    inline vk::Device get_device() const
    {
        return m_device;
    }

    inline vk::CommandBuffer get_transfer_buffer() const
    {
        return m_transfer_buffer;
    }

    inline vk::Queue get_graphics_queue() const
    {
        return m_graphics_queue;
    }

    inline PipelineCache& get_pipeline_cache()
    {
        return m_pipeline_cache;
    }

    inline SamplerCache& get_sampler_cache()
    {
        return m_sampler_cache;
    }

    inline RenderPassCache& get_render_pass_cache()
    {
        return m_render_pass_cache;
    }

    inline std::mutex& get_graphics_mutex()
    {
        return m_graphics_mutex;
    }

    inline vk::Format get_surface_format()
    {
        return m_surface_format.format;
    }

private:
    static constexpr size_t max_frames_in_flight = 2;

    vk::Instance m_instance;
    vk::SurfaceKHR m_surface;

    vk::PhysicalDevice m_physical_device;
    vk::PhysicalDeviceProperties m_physical_device_properties;
    vk::Device m_device;
    vk::PhysicalDeviceMemoryProperties m_memory_properties;

    vk::Queue m_graphics_queue;
    uint32_t m_graphics_queue_index = 0;
    std::mutex m_graphics_mutex;

    vk::Queue m_compute_queue;
    uint32_t m_compute_queue_index = 0;
    std::mutex m_compute_mutex;

    vk::SurfaceCapabilitiesKHR m_surface_capabilities;
    std::vector<vk::PresentModeKHR> m_surface_present_modes;
    vk::SurfaceFormatKHR m_surface_format;

    vk::CommandPool m_graphics_command_pool;
    vk::CommandBuffer m_transfer_buffer;

    vk::QueryPool m_timestamp_query_pool;

    PipelineCache m_pipeline_cache;
    SamplerCache m_sampler_cache;
    RenderPassCache m_render_pass_cache;
    FramebufferCache m_framebuffer_cache;
    RenderGraphCache m_render_graph_cache;
    StagingBufferPool m_buffer_pool;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;

    // Frame in flight resources
    std::array<vk::CommandBuffer, max_frames_in_flight> m_command_buffers;
    std::array<vk::Semaphore, max_frames_in_flight> m_acquire_semaphores;
    std::array<vk::Fence, max_frames_in_flight> m_frame_fences;
    std::vector<vk::Semaphore> m_submit_semaphores;
    size_t m_current_frame = 0;

    uint32_t m_frames_limit = 0;
    // Time between two frames in microseconds when `m_frames_limit != 0`.
    uint32_t m_time_between_frames = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_last_frame_limit_time;

    TracyVkCtx m_tracy_context = {};
    SDL_Window *m_window = nullptr;

    static PFN_vkVoidFunction imgui_get_proc_addr(const char *name, void *user)
    {
        (void)user;
        return vk::detail::defaultDispatchLoaderDynamic.vkGetInstanceProcAddr(RenderingDriverVulkan::get()->m_instance, name);
    }

    // Swapchain resources
    uint32_t m_swapchain_image_count;
    vk::SwapchainKHR m_swapchain = nullptr;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<Ref<Texture>> m_swapchain_textures;

    Result<Ref<Texture>> create_texture_from_vk_image(vk::Image image, uint32_t width, uint32_t height, vk::Format format);

    void destroy_swapchain();

    std::optional<uint32_t> find_memory_type_index(uint32_t type_bits, vk::MemoryPropertyFlags properties);
    Result<vk::DeviceMemory> allocate_memory_for_buffer(vk::Buffer buffer, vk::MemoryPropertyFlags properties);
    Result<vk::DeviceMemory> allocate_memory_for_image(vk::Image image, vk::MemoryPropertyFlags properties);

    Result<QueueInfo, bool> find_queue(vk::PhysicalDevice physical_device);
    std::optional<PhysicalDeviceWithInfo> pick_best_device(const std::vector<vk::PhysicalDevice>& physical_devices, const std::vector<const char *>& required_extensions, const std::vector<const char *>& optional_extensions);
};

class BufferVulkan : public Buffer
{
    CLASS(BufferVulkan, Buffer);

public:
    BufferVulkan(vk::Buffer buffer, vk::DeviceMemory memory, size_t size)
        : buffer(buffer), memory(memory)
    {
        m_size = size;
    }

    virtual ~BufferVulkan();

    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

class TextureVulkan : public Texture
{
    CLASS(TextureVulkan, Texture);

public:
    TextureVulkan(vk::Image image, vk::DeviceMemory memory, vk::ImageView image_view, uint32_t width, uint32_t height, size_t size, vk::ImageAspectFlags aspect_mask, uint32_t layers, bool owned)
        : image(image), memory(memory), image_view(image_view), size(size), aspect_mask(aspect_mask), layers(layers), owned(owned)
    {
        m_width = width;
        m_height = height;
        m_layout = TextureLayout::Undefined;
    }

    virtual ~TextureVulkan();

    virtual void update(View<uint8_t> view, uint32_t layer) override;
    virtual void transition_layout(TextureLayout new_layout) override;

    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView image_view;
    size_t size;

    vk::ImageAspectFlags aspect_mask;
    uint32_t layers;

    // Does the texture owned the `vk::Image` ?
    bool owned;
};

class MeshVulkan : public Mesh
{
    CLASS(MeshVulkan, Mesh);

public:
    MeshVulkan(IndexType index_type, vk::IndexType index_type_vk, size_t vertex_count, Ref<Buffer> index_buffer, Ref<Buffer> vertex_buffer, Ref<Buffer> normal_buffer, Ref<Buffer> uv_buffer)
        : index_buffer(index_buffer), vertex_buffer(vertex_buffer), normal_buffer(normal_buffer), uv_buffer(uv_buffer), index_type_vk(index_type_vk)
    {
        this->m_index_type = index_type;
        this->m_vertex_count = vertex_count;
    }

    virtual ~MeshVulkan() {}

    Ref<Buffer> index_buffer;
    Ref<Buffer> vertex_buffer;
    Ref<Buffer> normal_buffer;
    Ref<Buffer> uv_buffer;

    vk::IndexType index_type_vk;
};

class DescriptorPool
{
public:
    [[nodiscard]]
    static Result<DescriptorPool> create(vk::DescriptorSetLayout layout, Ref<Shader> shader);

    [[nodiscard]]
    Result<vk::DescriptorSet> allocate();

    DescriptorPool()
        : m_allocation_count(0)
    {
    }

    ~DescriptorPool();

private:
    DescriptorPool(vk::DescriptorSetLayout layout, const std::vector<vk::DescriptorPoolSize>&& sizes)
        : m_layout(layout), m_sizes(sizes), m_allocation_count(0)
    {
    }

    Result<> add_pool();

    static constexpr uint32_t max_sets = 8;

    vk::DescriptorSetLayout m_layout;
    std::vector<vk::DescriptorPool> m_pools;
    std::vector<vk::DescriptorPoolSize> m_sizes;
    uint32_t m_allocation_count;
};

class MaterialLayoutVulkan : public MaterialLayout
{
    CLASS(MaterialLayoutVulkan, MaterialLayout);

public:
    MaterialLayoutVulkan(vk::DescriptorSetLayout m_descriptor_set_layout, DescriptorPool descriptor_pool, Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, vk::PolygonMode polygon_mode, vk::CullModeFlags cull_mode, MaterialFlags flags, vk::PipelineLayout pipeline_layout)
        : m_descriptor_pool(descriptor_pool), m_descriptor_set_layout(m_descriptor_set_layout), m_shader(shader), m_instance_layout(instance_layout), m_polygon_mode(polygon_mode), m_cull_mode(cull_mode), m_flags(flags), m_pipeline_layout(pipeline_layout)
    {
    }

    virtual ~MaterialLayoutVulkan();

    // private:
    DescriptorPool m_descriptor_pool;
    vk::DescriptorSetLayout m_descriptor_set_layout;

    Ref<Shader> m_shader;
    std::optional<InstanceLayout> m_instance_layout;
    vk::PolygonMode m_polygon_mode;
    vk::CullModeFlags m_cull_mode;
    MaterialFlags m_flags;
    vk::PipelineLayout m_pipeline_layout;
};

class MaterialVulkan : public Material
{
    CLASS(MaterialVulkan, Material);

public:
    MaterialVulkan(Ref<MaterialLayout>& layout, vk::DescriptorSet descriptor_set)
        : descriptor_set(descriptor_set)
    {
        m_layout = layout;
    }

    virtual ~MaterialVulkan() {}

    virtual void set_param(const std::string& name, const Ref<Texture>& texture) override;
    virtual void set_param(const std::string& name, const Ref<Buffer>& buffer) override;

    vk::DescriptorSet descriptor_set;
};
