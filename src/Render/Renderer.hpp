#pragma once

#include "Core/Flags.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"
#include "stdext.hpp"

#include <webgpu/webgpu.h>

#include <limits>
#include <tuple>

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
class Chunk;
class ChunkPos;

struct RenderableChunk;

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

class Buffer
{
public:
    ~Buffer();

    static Result<std::shared_ptr<Buffer>> create(size_t size, WGPUBufferUsage usage = WGPUBufferUsage_None, BufferVisibility visibility = BufferVisibility::GPUOnly);

    template <typename T>
    void update_struct(const T& value, size_t offset = 0)
    {
        std::array<T, 1> array{value};
        update(std::as_bytes(std::span(array)), offset);
    }

    void update(std::span<const std::byte> view, size_t offset = 0);

    size_t size() const { return m_size; }
    WGPUBufferUsage flags() const { return m_usage; }

    WGPUBuffer handle() const { return m_buffer; }

private:
    WGPUBuffer m_buffer;
    size_t m_size;
    WGPUBufferUsage m_usage;

    BufferVisibility m_visibility;
    WGPUBuffer m_transfer_buffer;
};

class Texture
{
public:
    ~Texture();

    static Result<std::shared_ptr<Texture>> create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage = WGPUTextureUsage_None, WGPUTextureViewDimension dimension = WGPUTextureViewDimension_2D, uint32_t layers = 1, uint32_t mip_level = 1);
    static std::shared_ptr<Texture> create_from_handle(WGPUTexture texture, WGPUTextureView view);
    static Result<std::shared_ptr<Texture>> load(std::string_view path);

    void update(std::span<const std::byte> view, uint32_t layer = 0);

    WGPUTexture handle() const { return m_texture; }
    WGPUTextureView handle_view() const { return m_view; }
    WGPUTextureFormat format() const { return m_format; }

private:
    WGPUTexture m_texture = nullptr;
    WGPUTextureView m_view = nullptr;
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_layers = 0;
    uint32_t m_mip_level = 0;
    bool m_external = false;
};

class Mesh
{
public:
    enum class BufferKind
    {
        Index = 0,
        Position = 1,
        Normal = 2,
        UV = 3,
        Max,
    };

    static Result<std::shared_ptr<Mesh>> create_from_data(std::span<const std::byte> index, std::span<const glm::vec3> positions, std::span<const glm::vec3> normals, std::span<const std::byte> uvs, WGPUIndexFormat index_type = WGPUIndexFormat_Uint32, WGPUVertexFormat uv_format = WGPUVertexFormat_Float32x2);

    Mesh(uint32_t vertex_count, WGPUIndexFormat index_type, WGPUVertexFormat uv_format, const std::shared_ptr<Buffer>& index_buffer, const std::shared_ptr<Buffer>& position_buffer, const std::shared_ptr<Buffer>& normal_buffer, const std::shared_ptr<Buffer>& uv_buffer)
        : m_vertex_count(vertex_count), m_index_type(index_type), m_uv_format(uv_format)
    {
        set_buffer(BufferKind::Index, index_buffer);
        set_buffer(BufferKind::Position, position_buffer);
        set_buffer(BufferKind::Normal, normal_buffer);
        set_buffer(BufferKind::UV, uv_buffer);
    }

    ALWAYS_INLINE uint32_t vertex_count() const { return m_vertex_count; }

    ALWAYS_INLINE WGPUIndexFormat index_type() const { return m_index_type; }
    ALWAYS_INLINE WGPUVertexFormat uv_format() const { return m_uv_format; }

    ALWAYS_INLINE std::shared_ptr<Buffer> get_buffer(BufferKind kind) const
    {
        return m_buffers[(size_t)kind];
    }

    ALWAYS_INLINE void set_buffer(BufferKind kind, const std::shared_ptr<Buffer>& buffer)
    {
        m_buffers[(size_t)kind] = buffer;
    }

protected:
    uint32_t m_vertex_count;
    WGPUIndexFormat m_index_type;
    WGPUVertexFormat m_uv_format;
    std::shared_ptr<Buffer> m_buffers[(size_t)BufferKind::Max];
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

    bool operator<(const RenderTarget& r) const
    {
        return std::tie(format, blending) < std::tie(r.format, r.blending);
    }
};

struct RenderPass
{
    WGPURenderPassEncoder encoder;
    std::optional<RenderTarget> depth;
    std::vector<RenderTarget> textures;
};

struct InstanceAttribute
{
    uint32_t offset;
    WGPUVertexFormat format;
};

struct Instance
{
    std::vector<InstanceAttribute> attribs;
    size_t stride;
};

struct MaterialParamCache
{
    BindingKind kind = BindingKind::Texture;
    std::shared_ptr<Texture> texture = nullptr;
    std::shared_ptr<Buffer> buffer = nullptr;

    size_t offset = 0;
    size_t size = std::numeric_limits<size_t>::max();
};

enum class MaterialFlagBits
{
    None,
    Transparency = 1 << 0,
    Priority = 1 << 1,

    NoPosition = 1 << 2,
    NoNormal = 1 << 3,
    NoUV = 1 << 4,

    DisableDepthTest = 1 << 5,

    NoData = NoPosition | NoNormal | NoUV,
};
using MaterialFlags = Flags<MaterialFlagBits>;
DEFINE_FLAG_TRAITS(MaterialFlagBits);

class Material
{
public:
    struct PipelineKey
    {
        std::vector<RenderTarget> color_formats;
        std::optional<RenderTarget> depth_format;
        // WGPUCullMode cull_mode;

        bool operator<(const PipelineKey& k) const
        {
            if (depth_format < k.depth_format)
                return true;
            else if (k.depth_format < depth_format)
                return false;
            return color_formats < k.color_formats;
        }
    };

    ~Material();

    static std::shared_ptr<Material> create(const std::shared_ptr<Shader>& shader, MaterialFlags flags, WGPUCullMode cull_mode, WGPUVertexFormat uv_format, Instance instance = {});

    WGPURenderPipeline get_pipeline(const RenderPass& pass);

    std::shared_ptr<Shader> get_shader() const { return m_shader; }
    MaterialFlags flags() const { return m_flags; }
    WGPUCullMode get_cull_mode() const { return m_cull_mode; }
    size_t get_instance_stride() const { return m_instance_stride; }

    std::span<const InstanceAttribute> get_attributes() const { return m_attributes; }

private:
    std::shared_ptr<Shader> m_shader;

    MaterialFlags m_flags;
    WGPUCullMode m_cull_mode;
    WGPUVertexFormat m_uv_format;

    std::vector<InstanceAttribute> m_attributes;
    size_t m_instance_stride;

    std::map<PipelineKey, WGPURenderPipeline> m_pipelines;

    WGPURenderPipeline create_pipeline(const RenderPass& pass);
};

class BindGroup
{
public:
    static std::shared_ptr<BindGroup> create(const std::shared_ptr<Shader>& shader);

    ~BindGroup();

    void set_param(std::string_view name, const std::shared_ptr<Buffer>& buffer, size_t offset = 0, size_t size = std::numeric_limits<size_t>::max());
    void set_param(std::string_view name, const std::shared_ptr<Texture>& texture);

    WGPUBindGroup get_bind_group();

private:
    std::shared_ptr<Shader> m_shader;
    stdext::string_map<MaterialParamCache> m_caches;
    WGPUBindGroup m_bind_group = nullptr;

    bool m_dirty = true;

    const MaterialParamCache& get_param(std::string_view name) const;
    void create_bind_group();
};

class SamplerCache
{
public:
    ~SamplerCache();
    WGPUSampler get(const SamplerDescriptor& desc);

private:
    std::map<SamplerDescriptor, WGPUSampler> m_samplers;
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
DEFINE_WGPU_HANDLE(Queue, WGPUQueue, wgpuQueueAddRef, wgpuQueueRelease);
}; // namespace wgpu

enum class WorldFlagBits
{
    Shadowmap = 1 << 0,
    NoFrustumCheck = 1 << 1, // TODO: remove in favor of specifying the frustum directly in `draw_world`.
    Water = 1 << 2,          // TODO: remove this water rendering needs its own function.
};
using WorldFlags = Flags<WorldFlagBits>;
DEFINE_FLAG_TRAITS(WorldFlagBits);

struct GPU_ATTRIBUTE CameraUniforms
{
    glm::mat4 view_matrix;
    glm::mat4 inv_view_matrix;
    glm::mat4 projection_matrix;
};

struct GPU_ATTRIBUTE ShadowmapCameraUniforms
{
    glm::mat4 view_projection;
};

struct GPU_ATTRIBUTE SSAOUniforms
{
    std::array<glm::vec4, 64> samples;
};

struct GPU_ATTRIBUTE FwChunkUniforms
{
    glm::mat4 model_matrix;
};

struct GPU_ATTRIBUTE FwCamera
{
    glm::mat4 view_projection;
};

struct GPU_ATTRIBUTE FwWorldEnv
{
    glm::mat4 light_view_projection;
    glm::vec3 light_dir;
};

struct GPU_ATTRIBUTE FwColored
{
    glm::mat4 model;
    glm::vec4 color;
};

struct GPU_ATTRIBUTE SkyUniforms
{
    glm::vec4 color;
};

struct GPU_ATTRIBUTE PostProcessUniforms
{
    glm::vec4 fog_color;
    float fog_distance;
    float near;
    float far;
    uint32_t underwater;
};

struct GPU_ATTRIBUTE CloudsParams
{
    glm::mat4 camera_projection;
    glm::mat4 camera_rot;
    glm::vec4 camera_position;
    glm::vec4 camera_dir;
    float aspect_ratio;
    float time;
    float near;
    float far;
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

    void draw_forward(const std::shared_ptr<World>& world);
    void draw_world(const std::shared_ptr<World>& world, const RenderPass& pass, WorldFlags flags, const std::span<const RenderableChunk>& chunks);
    void draw_all_world(const std::shared_ptr<World>& world, const RenderPass& pass, WorldFlags flags);
    void draw(const RenderPass& pass, const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const std::shared_ptr<BindGroup>& bg, const std::shared_ptr<Buffer>& instance_buffer = nullptr, size_t instance_count = 1);
    void draw_fullscreen(const RenderPass& pass, std::shared_ptr<Material> material, std::shared_ptr<BindGroup> bg);

    void set_fog(glm::vec4 color, float distance);
    void set_sky(glm::vec4 color);
    void set_underwater(bool v);

    std::shared_ptr<Shader> get_fw_chunk_shader() const { return m_fw_chunk_shader; }
    std::shared_ptr<Shader> get_fw_water_shader() const { return m_fw_water_shader; }
    std::shared_ptr<Shader> get_fw_shadowmap_shader() const { return m_fw_chunk_shadowmap_shader; }
    std::shared_ptr<Shader> get_fw_item_block_shader() const { return m_fw_item_block_shader; }
    std::shared_ptr<Shader> get_fw_item_shader() const { return m_fw_item_shader; }
    std::shared_ptr<Shader> get_fw_text_shader() const { return m_fw_text_shader; }
    std::shared_ptr<Shader> get_fw_model_shader() const { return m_fw_model_shader; }
    std::shared_ptr<Material> get_fw_chunk_mat() const { return m_fw_chunk_mat; }
    std::shared_ptr<Material> get_fw_shadowmap_mat() const { return m_fw_chunk_shadowmap_mat; }
    std::shared_ptr<Material> get_fw_texture_rect_mat() const { return m_fw_texture_rect_mat; }
    std::shared_ptr<Material> get_fw_model_mat() const { return m_fw_model_mat; }
    std::shared_ptr<Material> get_fw_text_mat() const { return m_fw_text_mat; }
    std::shared_ptr<Material> get_fw_color_rect_mat() const { return m_fw_color_rect_mat; }
    std::shared_ptr<Material> get_fw_item_block_mat() const { return m_fw_item_block_mat; }
    std::shared_ptr<Material> get_fw_item_mat() const { return m_fw_item_mat; }
    std::shared_ptr<Texture> get_fw_shadowmap() const { return m_fw_shadowmap; }
    std::shared_ptr<Buffer> get_fw_camera() const { return m_fw_camera; }
    std::shared_ptr<Buffer> get_fw_camera_rel() const { return m_fw_camera_rel; }
    std::shared_ptr<Buffer> get_fw_world_env() const { return m_fw_world_env; }
    std::shared_ptr<Buffer> get_fw_shadowmap_camera() const { return m_fw_shadowmap_camera; }

    std::shared_ptr<Texture> get_fw_water_texture() const { return m_fw_water_texture; }

    WGPUSampler get_sampler(const SamplerDescriptor& desc) { return m_sampler_cache.get(desc); }

    WGPUDevice device() const { return m_device; }
    WGPUTextureFormat get_surface_format() const { return m_surface_format; }
    Extent2D get_surface_extent() const { return m_surface_extent; }

    WGPUQueue get_queue() const { return m_queue; }

    std::shared_ptr<Buffer> get_env_2d() const { return m_env_2d_buffer; }

    std::mutex& get_device_mutex() { return m_device_mutex; }
    std::mutex& get_queue_mutex() { return m_queue_mutex; }

    std::shared_ptr<Mesh> get_cube_mesh() const { return m_cube_mesh; }
    std::shared_ptr<Mesh> get_square_mesh() const { return m_square_mesh; }
    std::shared_ptr<Mesh> get_quad_mesh() const { return m_quad_mesh; }

    std::shared_ptr<Shader> get_preview_block_shader() const { return m_preview_block_shader; }
    std::shared_ptr<Shader> get_color_rect_shader() const { return m_color_rect_shader; }
    std::shared_ptr<Shader> get_texture_rect_shader() const { return m_texture_rect_shader; }

    std::shared_ptr<Texture> get_missing_texture() const { return m_missing_texture; }
    std::span<const uint8_t> get_missing_texture_data() const;

    size_t get_device_memory_usage() const { return m_device_memory_allocated - m_device_memory_freed; }
    size_t get_pipeline_count() const { return 0; }

    static ALWAYS_INLINE Renderer& get() { return *singleton; }

private:
    wgpu::Instance m_instance = nullptr;
    wgpu::Adapter m_adapter = nullptr;
    wgpu::Device m_device = nullptr;
    wgpu::Surface m_surface = nullptr;
    wgpu::Queue m_queue = nullptr;

    std::mutex m_device_mutex;
    std::mutex m_queue_mutex;

    WGPUQuerySet m_occlusion_set = nullptr;

    // Forward rendering
    std::shared_ptr<Texture> m_fw_depth_texture;
    std::shared_ptr<Texture> m_fw_color_texture;

    std::shared_ptr<Buffer> m_fw_camera;
    std::shared_ptr<Buffer> m_fw_camera_rel;
    std::shared_ptr<Buffer> m_fw_world_env;

    std::shared_ptr<Shader> m_fw_chunk_shadowmap_shader;

    std::shared_ptr<Texture> m_fw_shadowmap;
    std::shared_ptr<Buffer> m_fw_shadowmap_camera;

    std::shared_ptr<Shader> m_fw_chunk_shader;
    std::shared_ptr<Shader> m_fw_water_shader;
    std::shared_ptr<Shader> m_fw_item_block_shader;
    std::shared_ptr<Shader> m_fw_item_shader;
    std::shared_ptr<Shader> m_fw_text_shader;
    std::shared_ptr<Shader> m_fw_colored_shader;
    std::shared_ptr<Shader> m_fw_model_shader;

    std::shared_ptr<Material> m_fw_chunk_mat;
    std::shared_ptr<Material> m_fw_chunk_shadowmap_mat;
    std::shared_ptr<Material> m_fw_water_mat;
    std::shared_ptr<Material> m_fw_texture_rect_mat;
    std::shared_ptr<Material> m_fw_text_mat;
    std::shared_ptr<Material> m_fw_model_mat;
    std::shared_ptr<Material> m_fw_color_rect_mat;
    std::shared_ptr<Material> m_fw_item_block_mat;
    std::shared_ptr<Material> m_fw_item_mat;

    std::shared_ptr<Buffer> m_fw_colored_buffer;
    std::shared_ptr<Material> m_fw_colored_mat;
    std::shared_ptr<Material> m_fw_colored_shadowmap_mat;
    std::shared_ptr<BindGroup> m_fw_colored_bg;
    std::shared_ptr<BindGroup> m_fw_colored_shadowmap_bg;

    std::shared_ptr<Material> m_fw_shadowmap_cam_mat;
    std::shared_ptr<BindGroup> m_fw_shadowmap_cam_bg;
    std::shared_ptr<Buffer> m_fw_shadowmap_cam_buffer;

    std::shared_ptr<Texture> m_fw_water_texture;

    // SSAO
    std::shared_ptr<Texture> m_ssao_buffer;
    std::shared_ptr<Buffer> m_ssao_uniform_buffer;
    std::shared_ptr<Shader> m_ssao_shader;
    std::shared_ptr<Material> m_ssao_material;
    std::shared_ptr<Texture> m_ssao_noise_texture;

    // Sky
    std::shared_ptr<Buffer> m_sky_buffer;
    std::shared_ptr<Shader> m_sky_shader;
    std::shared_ptr<Material> m_sky_mat;
    std::shared_ptr<BindGroup> m_sky_bg;

    // Clouds
    std::shared_ptr<Buffer> m_fw_clouds_buffer;
    std::shared_ptr<Shader> m_fw_clouds_shader;
    std::shared_ptr<Material> m_fw_clouds_mat;
    std::shared_ptr<BindGroup> m_fw_clouds_bg;
    std::shared_ptr<Texture> m_fw_clouds_noise;

    // Post processing
    std::shared_ptr<Shader> m_fw_pp_shader;
    std::shared_ptr<Material> m_fw_pp_mat;
    PostProcessUniforms m_fw_pp{};
    std::shared_ptr<Buffer> m_fw_pp_buffer;

    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;
    Extent2D m_surface_extent;

    SamplerCache m_sampler_cache;

    std::shared_ptr<Shader> m_preview_block_shader;
    std::shared_ptr<Shader> m_color_rect_shader;
    std::shared_ptr<Shader> m_texture_rect_shader;

    std::shared_ptr<Mesh> m_cube_mesh;
    std::shared_ptr<Mesh> m_square_mesh;
    std::shared_ptr<Mesh> m_quad_mesh;

    std::shared_ptr<Buffer> m_env_2d_buffer;

    std::shared_ptr<Texture> m_missing_texture;

    std::atomic_size_t m_device_memory_allocated = 0;
    std::atomic_size_t m_device_memory_freed = 0;

    bool m_underwater_effect = false;

    static inline Renderer *singleton;
};
