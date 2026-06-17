#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/Definitions.hpp"
#include "Core/Flags.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"

#include <tuple>
#include <webgpu/webgpu.h>

#ifdef __platform_web
#define WGPU_STRING_VIEW_INIT nullptr
#define WGPU_STRING_VIEW(NAME) (NAME)
#define WGPUOptionalBool_True true
#else

#ifndef WGPU_STRING_VIEW_INIT
#define WGPU_STRING_VIEW_INIT {nullptr, 0}
#endif

#define WGPU_STRING_VIEW(NAME) {NAME, WGPU_STRLEN}
#endif

#include <atomic>
#include <mutex>

class World;

enum class InitFlagBits
{
    None = 0,
    Validation = 1 << 0,
};
typedef Flags<InitFlagBits> InitFlags;
DEFINE_FLAG_TRAITS(InitFlagBits);

enum class VSync : uint8_t
{
    Off,
    On,
};

enum class BufferVisibility : uint8_t
{
    GPUOnly,
    GPUAndCPU,
};

class Buffer : public Object
{
    CLASS(Buffer, Object);

public:
    ~Buffer();

    static Result<Ref<Buffer>> create(size_t size, WGPUBufferUsage usage = WGPUBufferUsage_None, BufferVisibility visibility = BufferVisibility::GPUOnly);

    void update(View<uint8_t> view, size_t offset = 0);

    size_t size() const { return m_size; }
    WGPUBufferUsage flags() const { return m_usage; }

    WGPUBuffer handle() const { return m_buffer; }

private:
    WGPUBuffer m_buffer;
    size_t m_size;
    WGPUBufferUsage m_usage;
};

class Texture : public Object
{
    CLASS(Texture, Object);

public:
    ~Texture();

    static Result<Ref<Texture>> create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage = WGPUTextureUsage_None, WGPUTextureViewDimension dimension = WGPUTextureViewDimension_2D, uint32_t layers = 1, uint32_t mip_level = 1);
    static Ref<Texture> create_from_handle(WGPUTexture texture, WGPUTextureView view);
    static Result<Ref<Texture>> load(const StringView& path);

    void update(View<uint8_t> view, uint32_t layer = 0);

    WGPUTexture handle() const { return m_texture; }
    WGPUTextureView handle_view() const { return m_view; }
    WGPUTextureFormat format() const { return m_format; }

private:
    WGPUTexture m_texture;
    WGPUTextureView m_view;
    WGPUTextureFormat m_format;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_layers;
    uint32_t m_mip_level;
    bool m_external = false;
};

class Mesh : public Object
{
    CLASS(Mesh, Object);

public:
    enum class BufferKind
    {
        Index = 0,
        Position = 1,
        Normal = 2,
        UV = 3,
        Max,
    };

    static Result<Ref<Mesh>> create_from_data(const View<uint8_t>& index, const View<glm::vec3>& positions, const View<glm::vec3>& normals, const View<uint8_t>& uvs, WGPUIndexFormat index_type = WGPUIndexFormat_Uint32, UVType uv_type = UVType::UV);

    Mesh(uint32_t vertex_count, WGPUIndexFormat index_type, UVType uv_type, const Ref<Buffer>& index_buffer, const Ref<Buffer>& position_buffer, const Ref<Buffer>& normal_buffer, const Ref<Buffer>& uv_buffer)
        : m_vertex_count(vertex_count), m_index_type(index_type), m_uv_type(uv_type)
    {
        set_buffer(BufferKind::Index, index_buffer);
        set_buffer(BufferKind::Position, position_buffer);
        set_buffer(BufferKind::Normal, normal_buffer);
        set_buffer(BufferKind::UV, uv_buffer);
    }

    ALWAYS_INLINE uint32_t vertex_count() const { return m_vertex_count; }

    ALWAYS_INLINE WGPUIndexFormat index_type() const { return m_index_type; }
    ALWAYS_INLINE UVType uv_type() const { return m_uv_type; }

    ALWAYS_INLINE Ref<Buffer> get_buffer(BufferKind kind) const
    {
        return m_buffers[(size_t)kind];
    }

    ALWAYS_INLINE void set_buffer(BufferKind kind, const Ref<Buffer>& buffer)
    {
        m_buffers[(size_t)kind] = buffer;
    }

protected:
    uint32_t m_vertex_count;
    WGPUIndexFormat m_index_type;
    UVType m_uv_type;
    Ref<Buffer> m_buffers[(size_t)BufferKind::Max];
};

struct RenderTarget
{
    RenderTarget(WGPUTextureFormat format)
        : format(format)
    {
    }

    RenderTarget(WGPUTextureFormat format, bool blending)
        : format(format), blending(blending)
    {
    }

    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    bool blending = true;

    bool operator==(const RenderTarget& r) const
    {
        return std::tie(format, blending) == std::tie(r.format, r.blending);
    }

    bool operator>(const RenderTarget& r) const
    {
        return std::tie(format, blending) > std::tie(r.format, r.blending);
    }
};

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    Option<RenderTarget> depth;
    Vector<RenderTarget> textures;
};

struct InstanceAttribute
{
    uint32_t offset;
    WGPUVertexFormat format;
};

struct Instance
{
    Vector<InstanceAttribute> attribs;
    size_t stride;
};

struct MaterialParamCache
{
    BindingKind kind = BindingKind::Texture;
    Ref<Texture> texture = nullptr;
    Ref<Buffer> buffer = nullptr;
};

enum class MaterialFlagBits
{
    None,
    Transparency = 1 << 0,
    Priority = 1 << 1,

    NoPosition = 1 << 2,
    NoNormal = 1 << 3,
    NoUV = 1 << 4,

    NoData = NoPosition | NoNormal | NoUV,
};
using MaterialFlags = Flags<MaterialFlagBits>;
DEFINE_FLAG_TRAITS(MaterialFlagBits);

class Material : public Object
{
    CLASS(Material, Object);

public:
    struct PipelineKey
    {
        Vector<RenderTarget> color_formats;
        Option<RenderTarget> depth_format;

        bool operator>(const PipelineKey& k) const
        {
            if (depth_format > k.depth_format)
                return true;
            else if (k.depth_format > depth_format)
                return false;
            return color_formats > k.color_formats;
        }

        bool operator==(const PipelineKey& k) const
        {
            return depth_format == k.depth_format && color_formats == k.color_formats;
        }
    };

    ~Material();

    static Result<Ref<Material>> create(const Ref<Shader>& shader, MaterialFlags flags, WGPUCullMode cull_mode, UVType uv_type, Instance instance = {});

    void set_param(const StringView& name, const Ref<Buffer>& buffer);
    void set_param(const StringView& name, const Ref<Texture>& texture);

    const MaterialParamCache& get_param(const StringView& name) const;

    WGPUBindGroup get_bind_group();
    WGPURenderPipeline get_pipeline(const RenderPass& pass);

    Ref<Shader> get_shader() const { return m_shader; }
    UVType get_uv_type() const { return m_uv_type; }
    MaterialFlags flags() const { return m_flags; }
    WGPUCullMode get_cull_mode() const { return m_cull_mode; }
    size_t get_instance_stride() const { return m_instance_stride; }

    Vector<InstanceAttribute> get_attributes() const { return m_attributes; }

private:
    Ref<Shader> m_shader;
    WGPUBindGroup m_bind_group = nullptr;

    HashMap<String, MaterialParamCache> m_caches;

    MaterialFlags m_flags;
    WGPUCullMode m_cull_mode;
    UVType m_uv_type;

    Vector<InstanceAttribute> m_attributes;
    size_t m_instance_stride;

    Map<PipelineKey, WGPURenderPipeline> m_pipelines;

    bool m_dirty = true;

    void create_bind_group();
    WGPURenderPipeline create_pipeline(const RenderPass& pass);
};

class SamplerCache
{
public:
    WGPUSampler get(const SamplerDescriptor& desc);
    void clear();

private:
    Map<SamplerDescriptor, WGPUSampler> m_samplers;
};

#define DEFINE_WGPU_HANDLE(name, handle_name, addref_func, release_func) \
    struct name                                                          \
    {                                                                    \
        name() : handle(nullptr) {}                                      \
        name(handle_name handle) : handle(handle) {}                     \
        name(const name& o) : handle(o.handle) {}                        \
        ~name() { release_func(handle); }                                \
        operator handle_name() const { return handle; }                  \
        void operator=(const handle_name& o) { handle = o; }             \
                                                                         \
        handle_name handle;                                              \
    }

namespace wgpu
{
DEFINE_WGPU_HANDLE(Instance, WGPUInstance, wgpuInstanceAddRef, wgpuInstanceRelease);
DEFINE_WGPU_HANDLE(Device, WGPUDevice, wgpuDeviceAddRef, wgpuDeviceRelease);
DEFINE_WGPU_HANDLE(Adapter, WGPUAdapter, wgpuAdapterAddRef, wgpuAdapterRelease);
DEFINE_WGPU_HANDLE(Surface, WGPUSurface, wgpuSurfaceAddRef, wgpuSurfaceRelease);
}; // namespace wgpu

struct GPU_ATTRIBUTE CameraUniforms
{
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

struct GPU_ATTRIBUTE WorldEnvironment
{
    glm::mat4 view_matrix = glm::identity<glm::mat4>();
    glm::vec3 sun_direction = glm::vec3();
    glm::vec3 sun_color = glm::vec4(1.0, 1.0, 1.0, 1.0);
};

struct GPU_ATTRIBUTE SimpleUniforms
{
    glm::mat4 model_matrix;
    glm::vec4 color;
};

struct GPU_ATTRIBUTE SkyUniforms
{
    glm::mat4 model_matrix;
    float time;
};

struct GPU_ATTRIBUTE FullscreenUniforms
{
    glm::mat4 projection_matrix;
    glm::mat4 model_matrix;
};

struct GPU_ATTRIBUTE SSAOUniforms
{
    Array<glm::vec4, 64> samples;
};

class Renderer
{
    friend class Buffer;
    friend class Texture;

public:
    Renderer();

    Result<void> init(const Window& window, InitFlags flags);

    void configure_surface(size_t width, size_t height, VSync vsync);

    // TODO: Only used by imgui for the main menu, which will be removed.
    void draw_legacy(std::function<void()> f);

    void draw(const Ref<World>& world);
    void draw(const RenderPass& pass, Ref<Mesh> mesh, Ref<Material> material, const Ref<Buffer>& instance_buffer = nullptr, size_t instance_count = 1);
    void draw_fullscreen(const RenderPass& pass, Ref<Material> material);
    void draw_world(const Ref<World>& world, const RenderPass& pass);

    void set_underwater(bool v) { m_underwater_effect = v; }

    WGPUSampler get_sampler(const SamplerDescriptor& desc) { return m_sampler_cache.get(desc); }

    WGPUDevice device() const { return m_device; }
    WGPUTextureFormat get_surface_format() const { return m_surface_format; }
    Extent2D get_surface_extent() const { return m_surface_extent; }

    WGPUQueue get_queue() const { return m_queue; }

    // Ref<Buffer> get_world_environment() const { return m_env_buffer; }
    Ref<Buffer> get_env_2d() const { return m_env_2d_buffer; }

    std::mutex& get_device_mutex() { return m_device_mutex; }
    std::mutex& get_queue_mutex() { return m_queue_mutex; }

    Ref<Shader> get_model_shader() const { return m_model_shader; }

    Ref<Mesh> get_cube_mesh() const { return m_cube_mesh; }
    Ref<Mesh> get_square_mesh() const { return m_square_mesh; }

    // Ref<Shader> get_simple_shader() const { return m_simple_shader; }
    Ref<Shader> get_preview_block_shader() const { return m_preview_block_shader; }
    // Ref<Shader> get_item_block_shader() const { return m_item_block_shader; }
    Ref<Shader> get_color_rect_shader() const { return m_color_rect_shader; }
    Ref<Shader> get_texture_rect_shader() const { return m_texture_rect_shader; }

    Ref<Shader> get_text_shader() const { return m_text_shader; }

    Ref<Texture> get_missing_texture() const { return m_missing_texture; }
    View<uint8_t> get_missing_texture_data() const;

    size_t get_device_memory_usage() const { return m_device_memory_allocated - m_device_memory_freed; }
    size_t get_pipeline_count() const { return 0; }

    static ALWAYS_INLINE Renderer& get() { return *singleton; }

private:
    wgpu::Instance m_instance = nullptr;
    wgpu::Adapter m_adapter = nullptr;
    wgpu::Device m_device = nullptr;
    wgpu::Surface m_surface = nullptr;
    WGPUQueue m_queue = nullptr;

    std::mutex m_device_mutex;
    std::mutex m_queue_mutex;

    // Rendering 2.0 stuff
    Ref<Texture> m_albedo_buffer;
    Ref<Texture> m_position_buffer;
    Ref<Texture> m_normal_buffer;
    Ref<Texture> m_depth_buffer;

    Ref<Buffer> m_camera_buffer;

    Ref<Shader> m_chunk_shader;
    Ref<Material> m_chunk_material;

    Ref<Shader> m_shading_shader;
    Ref<Material> m_shading_material;

    // SSAO
    Ref<Texture> m_ssao_buffer;
    Ref<Buffer> m_ssao_uniform_buffer;
    Ref<Shader> m_ssao_shader;
    Ref<Material> m_ssao_material;
    Ref<Texture> m_ssao_noise_texture;

    Ref<Shader> m_text_shader;

    // Rendering stuff
    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;
    Extent2D m_surface_extent;

    SamplerCache m_sampler_cache;

    Ref<Shader> m_model_shader;
    Ref<Shader> m_simple_shader;
    Ref<Shader> m_preview_block_shader;
    Ref<Shader> m_item_block_shader;
    Ref<Shader> m_color_rect_shader;
    Ref<Shader> m_texture_rect_shader;
    Ref<Shader> m_water_shader;

    Ref<Shader> m_underwater_shader;
    Ref<Material> m_underwater_material;

    Ref<Shader> m_sky_shader;
    Ref<Material> m_sky_material;
    SkyUniforms m_sky_uniforms{};
    Ref<Buffer> m_sky_uniform_buffer;

    Ref<Mesh> m_cube_mesh;
    Ref<Mesh> m_square_mesh;

    // Ref<Material> m_chunk_material;
    Ref<Material> m_water_material;
    Ref<Buffer> m_env_2d_buffer;

    // WorldEnvironment m_environment;
    // Ref<Buffer> m_env_buffer;

    Ref<Texture> m_missing_texture;

    std::atomic_size_t m_device_memory_allocated = 0;
    std::atomic_size_t m_device_memory_freed = 0;

    bool m_underwater_effect = false;

    static inline Renderer *singleton;
};
