#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"

#include <map>

#ifdef __platform_web
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu.h>
#else
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#endif

#ifdef __has_debug_menu
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#endif

class TextureWebGPU;
class MaterialWebGPU;

struct RenderPipelineCacheKey
{
    Ref<Material> material;
    std::vector<RenderPassColorAttachment> color_attachs;
    bool previous_depth_pass;

    RenderPipelineCacheKey(Ref<Material> material, std::vector<RenderPassColorAttachment> color_attachs, bool previous_depth_pass)
        : material(material), color_attachs(color_attachs), previous_depth_pass(previous_depth_pass)
    {
    }

    bool operator<(const RenderPipelineCacheKey& other) const
    {
        // FIXME : :D{":WAKDp/owoaksj fv;lnw GFukw ieush fvamilergthliBAOWFEVHI/WEuhj"}
        // if (previous_depth_pass >= other.previous_depth_pass)
        //     return false;
        // if (color_attachs.size() >= other.color_attachs.size())
        //     return false;
        // if (material >= other.material)
        //     return false;
        // return true;
        const void *m = other.material.ptr();
        const void *om = other.material.ptr();
        const bool color_pass = color_attachs.size() > 0;
        const bool other_color_pass = color_attachs.size() > 0;
        return std::tie(m, color_pass, previous_depth_pass) < std::tie(om, other_color_pass, other.previous_depth_pass);
    }
};

struct RenderPipelineCacheValue
{
    WGPUPipelineLayout pipeline_layout;
    WGPUBindGroupLayout bind_group_layout;
    WGPURenderPipeline pipeline;
};

class RenderPipelineCache2
{
public:
    RenderPipelineCacheValue get_or_create(Ref<MaterialWebGPU> material, Vector<RenderPassColorAttachment> color_attachments, bool load_depth);

    struct Key
    {
        uint32_t shader_hash;
        bool store_color;
        bool load_depth;
    };

private:
    Vector<std::pair<Key, RenderPipelineCacheValue>> m_pipelines;

    std::optional<RenderPipelineCacheValue> get(Key key);
};

class SamplerCache : public Cache<WGPUSampler, SamplerDescriptor>
{
public:
    SamplerCache() {}
    void clear();

    WGPUSampler create_object(const SamplerDescriptor& key) override;

private:
    std::map<SamplerDescriptor, WGPUSampler> m_samplers;
};

struct BindGroupCacheKey
{
    Ref<Material> material = nullptr;

    bool operator<(const BindGroupCacheKey& other) const
    {
        return material.ptr() < other.material.ptr();
    }
};

/**
 * With WebGPU we cannot write to bind group after creating them, which is a problem for us. So we create and destroy
 * bind groups when needed.
 */
class BindGroupCache
{
public:
    void clear();
    WGPUBindGroup get(Ref<Material> material);

private:
    std::map<BindGroupCacheKey, WGPUBindGroup> m_bind_groups;
};

class RenderGraphCache
{
public:
    struct Attachment
    {
        Ref<TextureWebGPU> texture = nullptr;
        bool surface_texture = false;
        bool depth_load_previous = false;
    };

    struct RenderPass
    {
        RenderPassDescriptor descriptor;
        std::vector<Attachment> attachments = {};
        std::optional<Attachment> depth_attachment = std::nullopt;
    };

    RenderPass& set_render_pass(uint32_t index, RenderPassDescriptor desc);
    RenderPass& get_render_pass(int32_t index);

    /**
        Destroy all resources. Should be called after the window is resized.
     */
    void clear();

private:
    std::vector<RenderPass> m_render_passes;
};

WGPUSurface create_surface(WGPUInstance instance, SDL_Window *window);

class RenderingDriverWebGPU : public RenderingDriver
{
    CLASS(RenderingDriverWebGPU, RenderingDriver);

    friend class TextureWebGPU;

public:
    RenderingDriverWebGPU();
    virtual ~RenderingDriverWebGPU() override;

    static RenderingDriverWebGPU *get()
    {
        return (RenderingDriverWebGPU *)RenderingDriver::get();
    }

    [[nodiscard]]
    virtual Result<void> initialize(const Window& window, InitFlags flags) override;

    [[nodiscard]]
    virtual Result<void> configure_surface(size_t width, size_t height, VSync vsync) override;

    virtual void poll() override;

    virtual void limit_frames(uint32_t limit) override;

    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer(size_t size, BufferUsageFlags usage = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) override;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage, TextureDimension dimension = TextureDimension::D2D, uint32_t layers = 1, uint32_t mip_level = 1) override;

    [[nodiscard]]
    virtual Ref<Material> create_material(const Ref<Shader>& shader, std::optional<InstanceLayout> instance_layout, MaterialFlags flags, PolygonMode polygon_mode, CullMode cull_mode, UVType uv_type) override;

    virtual void draw_graph(const RenderGraph& graph) override;

    Result<WGPURenderPipeline> create_render_pipeline(Ref<Shader> shader, UVType uv_type, std::optional<InstanceLayout> instance_layout, WGPUCullMode cull_mode, MaterialFlags flags, WGPUPipelineLayout pipeline_layout, const Vector<RenderPassColorAttachment>& color_attachs, bool previous_depth_pass);
    Result<WGPUComputePipeline> create_compute_pipeline(const Ref<Shader>& shader, WGPUPipelineLayout pipeline_layout);

    inline WGPUDevice get_device()
    {
        return m_device;
    }

    inline WGPUQueue get_queue()
    {
        return m_queue;
    }

    inline RenderPipelineCache2& get_pipeline_cache()
    {
        return m_pipeline_cache;
    }

    inline SamplerCache& get_sampler_cache()
    {
        return m_sampler_cache;
    }

    WGPUShaderModule create_shader_module(const Ref<Shader>& shader);

    WGPUSurface create_surface(WGPUInstance instance, SDL_Window *window);

private:
    WGPUInstance m_instance = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUQueue m_queue = nullptr;

    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;

    RenderPipelineCache2 m_pipeline_cache;
    SamplerCache m_sampler_cache;
    BindGroupCache m_bind_group_cache;
    RenderGraphCache m_render_graph_cache;

    WGPUBindGroupLayout m_mipmap_bind_group_layout = nullptr;
    WGPUPipelineLayout m_mipmap_layout = nullptr;
    WGPUComputePipeline m_mipmap_pipeline = nullptr;

#ifdef __platform_macos
    SDL_MetalView m_metal_view;
#endif
};

class BufferWebGPU : public Buffer
{
    CLASS(BufferWebGPU, Buffer);

public:
    BufferWebGPU(WGPUBuffer buffer, size_t size, BufferUsageFlags usage)
        : buffer(buffer)
    {
        m_size = size;
        m_usage = usage;
    }

    ~BufferWebGPU()
    {
        wgpuBufferRelease(buffer);
    }

    virtual void update(View<uint8_t> view, size_t offset) override;
    virtual void read_async(size_t offset, size_t size, BufferReadCallback callback, void *user) override;

    WGPUBuffer buffer;
};

class TextureWebGPU : public Texture
{
    CLASS(TextureWebGPU, Texture);

public:
    TextureWebGPU(WGPUTexture texture, WGPUTextureView view, uint32_t width, uint32_t height, TextureFormat format, uint32_t layers, uint32_t mip_level)
        : texture(texture), view(view), format(format)
    {
        m_width = width;
        m_height = height;
        m_layers = layers;
        m_mip_level = mip_level;
    }

    virtual ~TextureWebGPU()
    {
        wgpuTextureViewRelease(view);
        wgpuTextureRelease(texture);
    }

    virtual void update(View<uint8_t> view, uint32_t layer = 0) override;

    virtual void generate_mips() override;

    WGPUTexture texture;
    WGPUTextureView view;
    TextureFormat format;
};

class MaterialWebGPU : public Material
{
public:
    MaterialWebGPU(const Ref<Shader>& shader, std::optional<InstanceLayout> instance_layout, MaterialFlags flags, PolygonMode polygon_mode, CullMode cull_mode, UVType uv_type, WGPUPipelineLayout pipeline_layout, WGPUBindGroupLayout bind_group_layout)
        : Material(shader, instance_layout, flags, polygon_mode, cull_mode, uv_type), m_pipeline_layout(pipeline_layout), m_bind_group_layout(bind_group_layout)
    {
    }

    WGPUPipelineLayout pipeline_layout() const { return m_pipeline_layout; }
    WGPUBindGroupLayout bind_group_layout() const { return m_bind_group_layout; }

private:
    WGPUPipelineLayout m_pipeline_layout;
    WGPUBindGroupLayout m_bind_group_layout;
};
