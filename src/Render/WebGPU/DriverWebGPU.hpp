#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"

#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#ifdef __platform_web
#include <emscripten.h>
#include <emscripten/html5_webgpu.h>
#endif

#include <map>

class TextureWebGPU;

class PipelineCache
{
public:
    struct Key
    {
        Material *material;

        bool operator<(const Key& key) const
        {
            return key.material < material;
        }
    };

    PipelineCache();

    Expected<wgpu::RenderPipeline> get_or_create(Material *material);

private:
    std::map<Key, wgpu::RenderPipeline> m_pipelines;
};

class SamplerCache
{
public:
    SamplerCache();

    Expected<wgpu::Sampler> get_or_create(Sampler sampler);

private:
    std::map<Sampler, wgpu::Sampler> m_samplers;
};

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
    virtual Expected<void> initialize(const Window& window) override;

    [[nodiscard]]
    virtual Expected<void> configure_surface(const Window& window, VSync vsync) override;

    virtual void poll() override;

    virtual void limit_frames(uint32_t limit) override;

    [[nodiscard]]
    virtual Expected<Ref<Buffer>> create_buffer(size_t size, BufferUsage usage = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) override;

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) override;

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers) override;

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) override;

    [[nodiscard]]
    virtual Expected<Ref<Mesh>> create_mesh(IndexType index_type, Span<uint8_t> indices, Span<glm::vec3> vertices, Span<glm::vec2> uvs, Span<glm::vec3> normals) override;

    [[nodiscard]]
    virtual Expected<Ref<MaterialLayout>> create_material_layout(Ref<Shader> shader, MaterialFlags flags = {}, std::optional<InstanceLayout> instance_layout = std::nullopt, CullMode cull_mode = CullMode::Back, PolygonMode polygon_mode = PolygonMode::Fill) override;

    [[nodiscard]]
    virtual Expected<Ref<Material>> create_material(MaterialLayout *layout) override;

    virtual void draw_graph(const RenderGraph& graph) override;

    Expected<wgpu::RenderPipeline> create_render_pipeline(Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, wgpu::CullMode cull_mode, MaterialFlags flags, wgpu::PipelineLayout pipeline_layout);

    inline wgpu::Device get_device()
    {
        return m_device;
    }

    inline wgpu::Queue get_queue()
    {
        return m_queue;
    }

    inline PipelineCache& get_pipeline_cache()
    {
        return m_pipeline_cache;
    }

    inline SamplerCache& get_sampler_cache()
    {
        return m_sampler_cache;
    }

    inline wgpu::BindGroupLayout get_push_constant_layout()
    {
        return m_push_constant_layout;
    }

private:
    wgpu::Instance m_instance;
    wgpu::Device m_device;
    wgpu::Surface m_surface;
    wgpu::Queue m_queue;

    wgpu::BindGroupLayout m_push_constant_layout;

    wgpu::TextureFormat m_surface_format;
    Ref<TextureWebGPU> m_depth_texture;

    PipelineCache m_pipeline_cache;
    SamplerCache m_sampler_cache;
};

class BufferWebGPU : public Buffer
{
    CLASS(BufferWebGPU, Buffer);

public:
    BufferWebGPU(wgpu::Buffer buffer, size_t size)
        : buffer(buffer)
    {
        m_size = size;
    }

    virtual void update(Span<uint8_t> view, size_t offset = 0) override;

    wgpu::Buffer buffer;
};

class TextureWebGPU : public Texture
{
    CLASS(TextureWebGPU, Texture);

public:
    TextureWebGPU(wgpu::Texture texture, wgpu::TextureView view, uint32_t width, uint32_t height, TextureFormat format)
        : texture(texture), view(view), format(format)
    {
        m_width = width;
        m_height = height;
    }

    virtual void update(Span<uint8_t> view, uint32_t layer = 0) override;

    virtual void transition_layout(TextureLayout new_layout) override
    {
        (void)new_layout;
    }

    wgpu::Texture texture;
    wgpu::TextureView view;
    TextureFormat format;
};

class MeshWebGPU : public Mesh
{
    CLASS(MeshWebGPU, Mesh);

public:
    MeshWebGPU(IndexType index_type, uint32_t vertex_count, Ref<Buffer> index_buffer, Ref<Buffer> vertex_buffer, Ref<Buffer> normal_buffer, Ref<Buffer> uv_buffer)
        : index_type(index_type), index_buffer(index_buffer), vertex_buffer(vertex_buffer), normal_buffer(normal_buffer), uv_buffer(uv_buffer)
    {
        m_vertex_count = vertex_count;
    }

    IndexType index_type;
    Ref<Buffer> index_buffer;
    Ref<Buffer> vertex_buffer;
    Ref<Buffer> normal_buffer;
    Ref<Buffer> uv_buffer;
};

class MaterialLayoutWebGPU : public MaterialLayout
{
    CLASS(MaterialLayoutWebGPU, MaterialLayout);

public:
    MaterialLayoutWebGPU(wgpu::BindGroupLayout layout, Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, wgpu::CullMode cull_mode, MaterialFlags flags, wgpu::PipelineLayout pipeline_layout)
        : layout(layout), shader(shader), instance_layout(instance_layout), cull_mode(cull_mode), flags(flags), pipeline_layout(pipeline_layout)
    {
    }

    wgpu::BindGroupLayout layout;
    Ref<Shader> shader;
    std::optional<InstanceLayout> instance_layout;
    wgpu::CullMode cull_mode;
    MaterialFlags flags;
    wgpu::PipelineLayout pipeline_layout;
};

class MaterialWebGPU : public Material
{
    CLASS(MaterialWebGPU, Material);

public:
    MaterialWebGPU(Ref<MaterialLayout> layout, Ref<Buffer> push_constant_buffer, wgpu::BindGroup push_constant)
        : push_constant_buffer(push_constant_buffer), push_constant_bind_group(push_constant)
    {
        m_layout = layout;
    }

    union ParamCache
    {
        BindingKind kind;
        struct
        {
            BindingKind kind;
            TextureWebGPU *texture;
        } texture;
        struct
        {
            BindingKind kind;
            BufferWebGPU *buffer;
        } buffer;
    };

    virtual void set_param(const std::string& name, const Ref<Texture>& texture) override;
    virtual void set_param(const std::string& name, const Ref<Buffer>& buffer) override;

    wgpu::BindGroup get_bind_group();

    std::map<std::string, ParamCache> caches;
    wgpu::BindGroup bind_group = nullptr;

    Ref<Buffer> push_constant_buffer;
    wgpu::BindGroup push_constant_bind_group = nullptr;

    /**
     * Indicate that the `bind_group` need to be recreated.
     */
    bool dirty = true;
};
