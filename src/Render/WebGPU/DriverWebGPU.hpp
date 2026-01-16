#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"

#include <map>

#ifdef __platform_web
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu.h>
#else
#include <webgpu/wgpu.h>
#endif

#ifdef __has_debug_menu
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>
#endif

class TextureWebGPU;

struct RenderPipelineCacheKey
{
    Ref<Material> material;
    std::vector<RenderPassColorAttachment> color_attachs;
    bool previous_depth_pass;

    bool operator<(const RenderPipelineCacheKey& other) const
    {
        return std::tie(material, color_attachs, previous_depth_pass) < std::tie(other.material, other.color_attachs, other.previous_depth_pass);
    }
};

class RenderPipelineCache : public Cache<WGPURenderPipeline, RenderPipelineCacheKey>
{
public:
    RenderPipelineCache() {}

    WGPURenderPipeline create_object(const RenderPipelineCacheKey& key) override;
};

struct ComputePipelineCacheKey
{
    Ref<ComputeMaterial> material = nullptr;

    bool operator<(const ComputePipelineCacheKey& other) const
    {
        return material.ptr() < other.material.ptr();
    }
};

class ComputePipelineCache : public Cache<WGPUComputePipeline, ComputePipelineCacheKey>
{
public:
    ComputePipelineCache() {}

    WGPUComputePipeline create_object(const ComputePipelineCacheKey& key) override;
};

class SamplerCache : public Cache<WGPUSampler, SamplerDescriptor>
{
public:
    SamplerCache() {}

    WGPUSampler create_object(const SamplerDescriptor& key) override;

private:
    std::map<SamplerDescriptor, WGPUSampler> m_samplers;
};

struct MaterialLayoutCacheKey
{
    Ref<MaterialBase> material = nullptr;

    bool operator<(const MaterialLayoutCacheKey& other) const
    {
        return material.ptr() < other.material.ptr();
    }
};

struct MaterialLayoutCacheValue
{
    WGPUBindGroupLayout bind_group_layout;
    WGPUPipelineLayout pipeline_layout;
};

class MaterialLayoutCache : public Cache<MaterialLayoutCacheValue, MaterialLayoutCacheKey>
{
protected:
    virtual MaterialLayoutCacheValue create_object(const MaterialLayoutCacheKey& key) override;
};

struct BindGroupCacheKey
{
    Ref<MaterialBase> material = nullptr;

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
    WGPUBindGroup get(Ref<MaterialBase> material);

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

public:
    RenderingDriverWebGPU();
    virtual ~RenderingDriverWebGPU() override;

    static RenderingDriverWebGPU *get()
    {
        return (RenderingDriverWebGPU *)RenderingDriver::get();
    }

    [[nodiscard]]
    virtual Result<> initialize(const Window& window, bool enable_validation) override;

    [[nodiscard]]
    virtual Result<> initialize_imgui(const Window& window) override;

    [[nodiscard]]
    virtual Result<> configure_surface(size_t width, size_t height, VSync vsync) override;

    virtual void poll() override;

    virtual void limit_frames(uint32_t limit) override;

    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer(const char *name, size_t size, BufferUsageFlags usage = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) override;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage, TextureDimension dimension = TextureDimension::D2D, uint32_t layers = 1) override;

    virtual void draw_graph(const RenderGraph& graph) override;

    Result<WGPURenderPipeline> create_render_pipeline(Ref<Shader> shader, UVType uv_type, std::optional<InstanceLayout> instance_layout, WGPUCullMode cull_mode, MaterialFlags flags, WGPUPipelineLayout pipeline_layout, const std::vector<RenderPassColorAttachment>& color_attachs, bool previous_depth_pass);
    Result<WGPUComputePipeline> create_compute_pipeline(const Ref<Shader>& shader, WGPUPipelineLayout pipeline_layout);

    inline WGPUDevice get_device()
    {
        return m_device;
    }

    inline WGPUQueue get_queue()
    {
        return m_queue;
    }

    inline RenderPipelineCache& get_pipeline_cache()
    {
        return m_pipeline_cache;
    }

    inline SamplerCache& get_sampler_cache()
    {
        return m_sampler_cache;
    }

    inline MaterialLayoutCache& get_material_layout_cache()
    {
        return m_material_layout_cache;
    }

    // inline WGPUBindGroupLayout get_push_constant_layout()
    // {
    //     return m_push_constant_layout;
    // }

    WGPUShaderModule create_shader_module(const Ref<Shader>& shader);

    WGPUSurface create_surface(WGPUInstance instance, SDL_Window *window);

private:
    WGPUInstance m_instance = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUQueue m_queue = nullptr;

    WGPUQuerySet m_timestamp_query_set = nullptr;

    // WGPUBindGroupLayout m_push_constant_layout; // TODO: Re-implement a push constant buffer to emulate them on web

    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;

    RenderPipelineCache m_pipeline_cache;
    ComputePipelineCache m_compute_pipeline_cache;
    SamplerCache m_sampler_cache;
    MaterialLayoutCache m_material_layout_cache;
    BindGroupCache m_bind_group_cache;
    RenderGraphCache m_render_graph_cache;

#ifdef __platform_macos
    SDL_MetalView m_metal_view;
#endif
};

class BufferWebGPU : public Buffer
{
    CLASS(BufferWebGPU, Buffer);

public:
    BufferWebGPU(WGPUBuffer buffer, Struct element, size_t size, BufferUsageFlags usage)
        : buffer(buffer)
    {
        m_element = element;
        m_size = size;
        m_usage = usage;
    }

    virtual void update(View<uint8_t> view, size_t offset) override;
    virtual void read_async(size_t offset, size_t size, BufferReadCallback callback, void *user) override;

    WGPUBuffer buffer;
};

class TextureWebGPU : public Texture
{
    CLASS(TextureWebGPU, Texture);

public:
    TextureWebGPU(WGPUTexture texture, WGPUTextureView view, uint32_t width, uint32_t height, TextureFormat format)
        : texture(texture), view(view), format(format)
    {
        m_width = width;
        m_height = height;
    }

    virtual ~TextureWebGPU() {}

    virtual void update(View<uint8_t> view, uint32_t layer = 0) override;

    WGPUTexture texture;
    WGPUTextureView view;
    TextureFormat format;
};
