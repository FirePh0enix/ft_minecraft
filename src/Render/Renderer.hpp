#pragma once

#include "Core/Flags.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Window.hpp"

#include <initializer_list>
#include <limits>

#ifdef __platform_web
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu.h>
#else
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#endif

#ifdef __platform_web
#define WGPU_STRING_VIEW_INIT nullptr
#define WGPU_STRING_VIEW(NAME) (NAME)
#define WGPUOptionalBool_True true
#else
#define WGPU_STRING_VIEW(NAME) {NAME, WGPU_STRLEN}
#endif

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_wgpu.h>

class World;

enum class RVDebugMessageValueType : uint32_t
{
    U32 = 0,
    I32 = 1,
};

struct RVDebugMessage
{
    glm::uvec4 value;
    RVDebugMessageValueType type;

    void dump() const
    {
        switch (type)
        {
        case RVDebugMessageValueType::U32:
            println("msg(u32) => {}", value.x);
            break;
        case RVDebugMessageValueType::I32:
            println("msg(i32) => {}", (int32_t)value.x);
            break;
        default:
            break;
        }
    }
};

struct RVDebugMessageInfo
{
    uint32_t count;
};

enum class RVVSync
{
    Off,
    On,
};

enum class RVInitFlagBits
{
    Validation = 1 << 0,
};
typedef Flags<RVInitFlagBits> RVInitFlags;
DEFINE_FLAG_TRAITS(RVInitFlagBits);

struct DispatchParams
{
    glm::mat4 inv_proj_matrix;
    glm::vec3 camera_origin;
};

class ShaderV2
{
public:
    static Result<ShaderV2> load(const std::string& path);

    const std::string& get_source_code() const { return m_source; }

private:
    ShaderV2(const std::string& source);

    std::string m_source;
};

/**
 *  Abstraction over a WGPUBuffer with some useful features missing from webgpu.
 */
struct Buffer
{
    WGPUBuffer buffer = nullptr;
    WGPUBuffer staging_buffer = nullptr;
    uint32_t size = 0;

    void clear(WGPUCommandEncoder command_encoder);
    void copy_to_staging(WGPUCommandEncoder command_encoder);

    /**
     *  Map the staging buffer to CPU memory.
     *  The returned pointer is read-only and modifying the content is undefined behavour.
     */
    const void *map(WGPUDevice device, uint32_t offset = 0, uint32_t size = std::numeric_limits<uint32_t>::max()) const;

    /**
     *  Unmap the buffer from CPU memory. `map` must have been previously called or this function results in undefined behavour.
     */
    void unmap() const;
};

struct ComputePipeline
{
    WGPUDevice device;
    WGPUComputePipeline pipeline;
    WGPUPipelineLayout pipeline_layout;
    std::vector<WGPUBindGroupLayout> bind_group_layouts;

    void create_bind_group(uint32_t index);
};

class ComputePipelineBuilder
{
public:
    ComputePipelineBuilder(WGPUDevice device) : m_device(device) {}

    ComputePipelineBuilder& set_shader(const std::string& path);
    ComputePipelineBuilder& add_group(std::initializer_list<WGPUBindGroupLayoutEntry> entries);
    ComputePipeline build();

private:
    struct BindGroupLayoutDescriptor
    {
        std::vector<WGPUBindGroupLayoutEntry> entries;
    };

    WGPUDevice m_device;
    String m_shader_source;
    std::vector<BindGroupLayoutDescriptor> m_bind_group_layout_descriptors;
};

class RVRenderer
{
public:
    RVRenderer();

    Result<> init(RVInitFlags flags, const Window& window);
    Result<> configure(const Window& window, RVVSync vsync);

    Result<Buffer> create_rw_storage_buffer(uint32_t size);

    void prepare_render();
    void render_world(Ref<World> world);

    void dump_messages();

public:
    WGPUInstance m_instance = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUQueue m_queue = nullptr;
    WGPUSurface m_surface = nullptr;

    WGPUTextureFormat m_surface_format = WGPUTextureFormat_Undefined;
    WGPUTextureFormat m_depth_format = WGPUTextureFormat_Depth32Float;
    WGPUTextureUsage m_surface_usage = WGPUTextureFormat_Undefined;
    WGPUCompositeAlphaMode m_alpha_mode = WGPUCompositeAlphaMode_Auto;
    bool m_support_immediate = false;
    Extent2D m_surface_extent = Extent2D();

    WGPUTexture m_albedo_texture = nullptr;
    WGPUTextureView m_albedo_texture_view = nullptr;
    WGPUTexture m_depth_texture = nullptr;
    WGPUTextureView m_depth_texture_view = nullptr;

    WGPUQuerySet m_query_set = nullptr;

    // rv_raytracing
    WGPUBindGroupLayout m_rv_raytracing_bind_group_layout = nullptr;
    WGPUBindGroupLayout m_rv_raytracing_node_bind_group_layout = nullptr;
    WGPUPipelineLayout m_rv_raytracing_pipeline_layout = nullptr;
    WGPUBindGroup m_rv_raytracing_bind_group = nullptr;
    WGPUComputePipeline m_rv_raytracing_pipeline = nullptr;

    DispatchParams m_rv_raytracing_params{};
    WGPUBuffer m_rv_raytracing_params_buffer = nullptr;

    // rv_dbg_messages
    size_t m_rv_dbg_messages_count;
    Buffer m_rv_dbg_messages;
    Buffer m_rv_dbg_messages_info;

    WGPUBindGroupLayout m_rv_dbg_messages_bind_group_layout;
    WGPUBindGroup m_rv_dbg_messages_bind_group;

    void create_rv_raytracing_resources();

    WGPUSurface create_surface(WGPUInstance instance, SDL_Window *window);
};
