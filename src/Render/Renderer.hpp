#pragma once

#include "Core/Class.hpp"
#include "Core/Definitions.hpp"
#include "Core/Flags.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"

#include "Render/WebGPU.hpp"

class World;

class RenderGraph;
class RenderPassNode;

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

    static Result<Ref<Texture>> create(uint32_t width, uint32_t height, WGPUTextureFormat format, WGPUTextureUsage usage = WGPUTextureUsage_None, TextureDimension dimension = TextureDimension::D2D, uint32_t layers = 1, uint32_t mip_level = 1);

    void update(View<uint8_t> view, uint32_t layer = 0);

    WGPUTexture handle() const { return m_texture; }
    WGPUTextureView handle_view() const { return m_view; }

private:
    WGPUTexture m_texture;
    WGPUTextureView m_view;
    WGPUTextureFormat m_format;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_layers;
    uint32_t m_mip_level;
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

struct InstanceAttribute
{
    uint32_t stride;
    WGPUVertexFormat format;
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
};
using MaterialFlags = Flags<MaterialFlagBits>;
DEFINE_FLAG_TRAITS(MaterialFlagBits);

class Material : public Object
{
    CLASS(Material, Object);

public:
    static Result<Ref<Material>> create(const Ref<Shader>& shader, MaterialFlags flags, WGPUPolygonMode polygon_mode, WGPUCullMode cull_mode, UVType uv_type, Vector<InstanceAttribute> attributes = {});

    void set_param(const StringView& name, const Ref<Buffer>& buffer);
    void set_param(const StringView& name, const Ref<Texture>& texture);

    const MaterialParamCache& get_param(const StringView& name) const;

    WGPUBindGroup get_bind_group();
    Ref<Shader> get_shader() const { return m_shader; }
    UVType get_uv_type() const { return m_uv_type; }
    MaterialFlags flags() const { return m_flags; }
    WGPUCullMode get_cull_mode() const { return m_cull_mode; }

    Vector<InstanceAttribute> get_attributes() const { return m_attributes; }

private:
    Ref<Shader> m_shader;
    WGPUBindGroup m_bind_group = nullptr;

    std::map<String, MaterialParamCache> m_caches;

    MaterialFlags m_flags;
    WGPUPolygonMode m_polygon_mode;
    WGPUCullMode m_cull_mode;
    UVType m_uv_type;

    Vector<InstanceAttribute> m_attributes;

    bool m_dirty = true;

    Result<void> create_bind_group();
};

class PipelineCache
{
public:
    struct Key
    {
        Ref<Shader> shader;
        WGPUBindGroupLayout bind_group_layout;
        UVType uv_type;
        MaterialFlags flags;
        WGPUCullMode cull_mode;
        Vector<InstanceAttribute> attributes;

        bool has_color_attach;
        bool has_depth_attach;

        bool operator<(const Key& k) const
        {
            return std::tie(shader, bind_group_layout, uv_type, flags, cull_mode, has_color_attach, has_depth_attach) < std::tie(k.shader, k.bind_group_layout, k.uv_type, k.flags, k.cull_mode, k.has_color_attach, k.has_depth_attach);
        }
    };

    Result<WGPURenderPipeline> get(const Key& key);

private:
    std::map<Key, WGPURenderPipeline> m_pipelines;
};

class SamplerCache
{
public:
    WGPUSampler get(const SamplerDescriptor& desc);

private:
    std::map<SamplerDescriptor, WGPUSampler> m_samplers;
};

struct GPU_ATTRIBUTE WorldEnvironment
{
    glm::mat4 view_matrix = glm::identity<glm::mat4>();
};

class Renderer
{
    friend class Buffer;
    friend class Texture;

public:
    Renderer();

    Result<void> init(const Window& window, InitFlags flags);
    Result<void> configure_surface(size_t width, size_t height, VSync vsync);

    void draw(RenderGraph& graph, std::function<void(const RenderPassNode& node)> f);
    void draw(RenderGraph& graph, Ref<World> world);

    WGPURenderPipeline get_pipeline(Ref<Material> material, const RenderPassNode& node);
    WGPUSampler get_sampler(const SamplerDescriptor& desc) { return m_sampler_cache.get(desc); }

    WGPUDevice device() const { return m_device; }
    WGPUTextureFormat get_surface_format() const { return m_surface_format; }
    Ref<Buffer> get_world_environment() const { return m_env_buffer; }

    Ref<Shader> get_voxel_shader() const { return m_voxel_shader; }
    Ref<Shader> get_model_shader() const { return m_model_shader; }

    Ref<Mesh> get_cube_mesh() const { return m_cube_mesh; }

    static ALWAYS_INLINE Renderer& get() { return *singleton; }

private:
    WGPUInstance m_instance = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUQueue m_queue = nullptr;

    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;
    Extent2D m_surface_extent;

    PipelineCache m_pipeline_cache;
    SamplerCache m_sampler_cache;

    Ref<Shader> m_voxel_shader;
    Ref<Shader> m_model_shader;

    Ref<Mesh> m_cube_mesh;

    Ref<Material> m_chunk_material;
    Ref<Buffer> m_env_buffer;

    static inline Renderer *singleton;

    static void record_world(Renderer& renderer, Ref<World> world, const RenderPassNode& node);
};
