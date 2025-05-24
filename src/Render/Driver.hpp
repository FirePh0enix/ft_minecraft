#pragma once

#include "Core/Error.hpp"
#include "Core/Ref.hpp"
#include "Core/Span.hpp"
#include "Render/Graph.hpp"
#include "Window.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

enum class VSync : uint8_t
{
    /**
     * @brief Disable vertical synchronization.
     */
    Off,

    /**
     * @brief Enable vertical synchronization.
     */
    On,
};

enum class BufferVisibility : uint8_t
{
    /**
     * @brief Indicate a buffer is only visible from GPU.
     */
    GPUOnly,

    /**
     * @brief Indicate a buffer visible from GPU and CPU.
     */
    GPUAndCPU,
};

struct BufferUsage
{
    /**
     * @brief Used as source in copy operation.
     */
    bool copy_src : 1 = false;

    /**
     * @brief Used as destination in copy operation.
     */
    bool copy_dst : 1 = false;

    /**
     * @brief Used as an uniform buffer.
     */
    bool uniform : 1 = false;

    /**
     * @brief Used as an index buffer.
     */
    bool index : 1 = false;

    /**
     * @brief Used as an vertex or instance buffer.
     */
    bool vertex : 1 = false;
};

enum class TextureFormat : uint8_t
{
    RGBA8Srgb,
    BGRA8Srgb,

    R32Sfloat,
    RG32Sfloat,
    RGB32Sfloat,
    RGBA32Sfloat,

    /**
     * @brief Depth 32-bits per pixel.
     */
    D32,
};

size_t size_of(const TextureFormat& format);

struct TextureUsage
{
    /**
     * @brief Used as source in copy operation.
     */
    bool copy_src : 1 = false;

    /**
     * @brief Used as destination in copy operation.
     */
    bool copy_dst : 1 = false;

    bool sampled : 1 = false;

    bool color_attachment : 1 = false;
    bool depth_attachment : 1 = false;
};

enum class IndexType : uint8_t
{
    Uint16,
    Uint32,
};

size_t size_of(const IndexType& format);

enum class PolygonMode : uint8_t
{
    Fill,
    Line,
    Point,
};

enum class CullMode : uint8_t
{
    None,
    Front,
    Back,
};

enum class ShaderType : uint8_t
{
    Float,
    Vec2,
    Vec3,
    Vec4,

    Uint,
};

class Buffer
{
public:
    /**
     * @brief Update the content of the buffer.
     */
    virtual void update(Span<uint8_t> view, size_t offset = 0) = 0;

    inline size_t size() const
    {
        return m_size;
    }

    virtual ~Buffer() {}

protected:
    size_t m_size;
};

// TODO: This probably should not be exposed.
enum class TextureLayout : uint8_t
{
    Undefined,
    DepthStencilAttachment,
    CopyDst,
    ShaderReadOnly,
    DepthStencilReadOnly,
};

class Texture
{
public:
    /**
     * @brief Update the content of a layer of the texture.
     */
    virtual void update(Span<uint8_t> view, uint32_t layer = 0) = 0;

    /**
     * @brief Change the layout of the texture.
     */
    virtual void transition_layout(TextureLayout new_layout) = 0;

    inline uint32_t width() const
    {
        return m_width;
    }

    inline uint32_t height() const
    {
        return m_height;
    }

    virtual ~Texture() {}

protected:
    uint32_t m_width;
    uint32_t m_height;
    TextureLayout m_layout;
};

class Mesh
{
public:
    inline uint32_t vertex_count()
    {
        return m_vertex_count;
    }

    inline IndexType index_type()
    {
        return m_index_type;
    }

    virtual ~Mesh() {}

protected:
    uint32_t m_vertex_count;
    IndexType m_index_type;
};

enum class ShaderKind : uint8_t
{
    Vertex,
    Fragment,
};

struct ShaderRef
{
    ShaderRef()
    {
    }

    ShaderRef(const char *filename, ShaderKind kind)
        : filename(filename), kind(kind)
    {
    }

    const char *filename;
    ShaderKind kind;
};

enum class MaterialParamKind : uint8_t
{
    Texture,
    UniformBuffer,
};

enum class Filter : uint8_t
{
    Linear,
    Nearest,
};

enum class AddressMode : uint8_t
{
    Repeat,
    ClampToEdge,
};

struct Sampler
{
    Filter min_filter = Filter::Linear;
    Filter mag_filter = Filter::Linear;

    struct
    {
        AddressMode u = AddressMode::Repeat;
        AddressMode v = AddressMode::Repeat;
        AddressMode w = AddressMode::Repeat;
    } address_mode = {};

    bool operator<(const Sampler& o) const
    {
        if ((uint32_t)min_filter >= (uint32_t)o.min_filter)
            return false;
        if ((uint32_t)mag_filter >= (uint32_t)o.mag_filter)
            return false;

        if ((uint32_t)address_mode.u >= (uint32_t)o.address_mode.u)
            return false;
        if ((uint32_t)address_mode.v >= (uint32_t)o.address_mode.v)
            return false;
        if ((uint32_t)address_mode.w >= (uint32_t)o.address_mode.w)
            return false;

        return true;
    }
};

union MaterialParam
{
    struct
    {
        MaterialParamKind kind;
        ShaderKind shader_kind;
        const char *name;
    };
    struct
    {
        MaterialParamKind kind;
        ShaderKind shader_kind;
        const char *name;
        Sampler sampler;
    } image_opts;

    static MaterialParam image(ShaderKind shader_kind, const char *name, Sampler sampler)
    {
        return {.image_opts = {.kind = MaterialParamKind::Texture, .shader_kind = shader_kind, .name = name, .sampler = sampler}};
    }

    static MaterialParam uniform_buffer(ShaderKind shader_kind, const char *name)
    {
        return {.kind = MaterialParamKind::UniformBuffer, .shader_kind = shader_kind, .name = name};
    }
};

struct InstanceLayoutInput
{
    ShaderType type;
    uint32_t offset;
};

struct InstanceLayout
{
    std::span<InstanceLayoutInput> inputs;
    size_t stride;

    InstanceLayout(std::span<InstanceLayoutInput> inputs, size_t stride)
        : inputs(inputs), stride(stride)
    {
    }
};

struct MaterialFlags
{
    bool transparency : 1 = false;
    bool always_first : 1 = false;
};

/**
 * @brief Describe the layout of a material which can be used to create multiple materials.
 *
 * This contains information about a type of material, allowing the reuse of information to create multiple
 * materials derived from it.
 */
class MaterialLayout
{
public:
    virtual ~MaterialLayout() {}
};

class Material
{
public:
    virtual void set_param(const std::string& name, Ref<Buffer>& buffer) = 0;
    virtual void set_param(const std::string& name, Ref<Texture>& texture) = 0;

    const Ref<MaterialLayout>& get_layout() const
    {
        return m_layout;
    }

    Ref<MaterialLayout>& get_layout()
    {
        return m_layout;
    }

    virtual ~Material() {}

protected:
    Ref<MaterialLayout> m_layout;
};

class RenderingDriver
{
public:
    RenderingDriver() {}
    virtual ~RenderingDriver() {}

    RenderingDriver(const RenderingDriver&) = delete;
    RenderingDriver(const RenderingDriver&&) = delete;
    RenderingDriver operator=(const RenderingDriver&) = delete;
    RenderingDriver operator=(const RenderingDriver&&) = delete;

    template <typename T>
    static void create_singleton()
    {
        singleton = make_ref<T>().template cast_to<RenderingDriver>();
    };

    /**
     * @brief Returns the singleton for the rendering driver.
     */
    static RenderingDriver *get()
    {
        return singleton.ptr();
    }

    /**
     * @brief Initialize the underlaying graphics API.
     */
    [[nodiscard]]
    virtual Expected<void> initialize(const Window& window) = 0;

    /**
     * @brief Configure the surface and swapchain.
     * It must be called every time the window is resized.
     */
    [[nodiscard]]
    virtual Expected<void> configure_surface(const Window& window, VSync vsync) = 0;

    /**
     * Limit the maximum number of frames per seconds. Set to `0` to remove the limit.
     */
    virtual void limit_frames(uint32_t limit) = 0;

    /**
     * @brief Allocate a buffer in the GPU memory.
     */
    [[nodiscard]]
    virtual Expected<Ref<Buffer>> create_buffer(size_t size, BufferUsage flags = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) = 0;

    /**
     * @brief Allocate a buffer in the GPU memory and fill it with `data`.
     */
    [[nodiscard]]
    virtual Expected<Ref<Buffer>> create_buffer_from_data(size_t size, Span<uint8_t> data, BufferUsage flags = {}, BufferVisibility visibility = BufferVisibility::GPUOnly);

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) = 0;

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers) = 0;

    [[nodiscard]]
    virtual Expected<Ref<Texture>> create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) = 0;

    [[nodiscard]]
    virtual Expected<Ref<Mesh>> create_mesh(IndexType index_type, Span<uint8_t> indices, Span<glm::vec3> vertices, Span<glm::vec2> uvs, Span<glm::vec3> normals) = 0;

    [[nodiscard]]
    virtual Expected<Ref<MaterialLayout>> create_material_layout(Span<ShaderRef> shaders, Span<MaterialParam> params = {}, MaterialFlags flags = {}, std::optional<InstanceLayout> instance_layout = std::nullopt, CullMode cull_mode = CullMode::Back, PolygonMode polygon_mode = PolygonMode::Fill, bool transparency = false, bool always_draw_before = false) = 0;

    [[nodiscard]]
    virtual Expected<Ref<Material>> create_material(MaterialLayout *layout) = 0;

    /**
     * @brief Draw a frame using a `RenderGraph`.
     */
    virtual void draw_graph(const RenderGraph& graph) = 0;

    inline Extent2D get_surface_extent() const
    {
        return m_surface_extent;
    }

protected:
    Extent2D m_surface_extent;

private:
    static Ref<RenderingDriver> singleton;
};
