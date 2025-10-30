#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Shader;
class RenderGraph;

typedef void (*BufferReadCallback)(const void *data, size_t size, void *user);

class Buffer : public Object
{
    CLASS(Buffer, Object);

public:
    virtual void update(View<uint8_t> view, size_t offset = 0) = 0;
    virtual void read_async(size_t offset, size_t size, BufferReadCallback callback, void *user) = 0;

    void read_async(size_t size, BufferReadCallback callback, void *user)
    {
        read_async(0, size, callback, user);
    }

    void read_async(BufferReadCallback callback, void *user)
    {
        read_async(0, size_bytes(), callback, user);
    }

    inline size_t size() const
    {
        return m_size;
    }

    inline size_t size_bytes() const
    {
        return m_size * m_element.size;
    }

    inline BufferUsageFlags flags() const
    {
        return m_usage;
    }

    const Struct& element() const
    {
        return m_element;
    }

    virtual ~Buffer() {}

protected:
    size_t m_size;
    Struct m_element;
    BufferUsageFlags m_usage;
};

class Texture : public Object
{
    CLASS(Texture, Object);

public:
    virtual ~Texture() {}

    /**
     * @brief Update the content of a layer of the texture.
     */
    virtual void update(View<uint8_t> view, uint32_t layer = 0) = 0;

    inline uint32_t width() const
    {
        return m_width;
    }

    inline uint32_t height() const
    {
        return m_height;
    }

protected:
    uint32_t m_width;
    uint32_t m_height;
    TextureLayout m_layout;
};

enum class MeshBufferKind
{
    Index = 0,
    Position = 1,
    Normal = 2,
    UV = 3,
    Max,
};

class Mesh : public Object
{
    CLASS(Mesh, Object);

public:
    static Ref<Mesh> create_from_data(const View<uint8_t>& index, const View<glm::vec3>& positions, const View<glm::vec3>& normals, const View<glm::vec2>& uvs, IndexType index_type = IndexType::Uint32);

    Mesh(uint32_t vertex_count, IndexType index_type, const Ref<Buffer>& index_buffer, const Ref<Buffer>& position_buffer, const Ref<Buffer>& normal_buffer, const Ref<Buffer>& uv_buffer)
        : m_vertex_count(vertex_count), m_index_type(index_type)
    {
        set_buffer(MeshBufferKind::Index, index_buffer);
        set_buffer(MeshBufferKind::Position, position_buffer);
        set_buffer(MeshBufferKind::Normal, normal_buffer);
        set_buffer(MeshBufferKind::UV, uv_buffer);
    }

    virtual ~Mesh() {}

    inline uint32_t vertex_count()
    {
        return m_vertex_count;
    }

    inline IndexType index_type()
    {
        return m_index_type;
    }

    ALWAYS_INLINE Ref<Buffer> get_buffer(MeshBufferKind kind) const
    {
        return m_buffers[(size_t)kind];
    }

    ALWAYS_INLINE void set_buffer(MeshBufferKind kind, const Ref<Buffer>& buffer)
    {
        m_buffers[(size_t)kind] = buffer;
    }

protected:
    uint32_t m_vertex_count;
    IndexType m_index_type;
    Ref<Buffer> m_buffers[(size_t)MeshBufferKind::Max];
};

struct InstanceLayoutInput
{
    ShaderType type;
    uint32_t offset;

    InstanceLayoutInput(ShaderType type, uint32_t offset)
        : type(type), offset(offset)
    {
    }
};

struct InstanceLayout
{
    std::vector<InstanceLayoutInput> inputs;
    size_t stride;

    InstanceLayout(View<InstanceLayoutInput> inputs, size_t stride)
        : inputs(inputs.to_vector()), stride(stride)
    {
    }
};

union MaterialParamCache
{
    BindingKind kind;
    struct
    {
        BindingKind kind;
        Texture *texture;
    } texture;
    struct
    {
        BindingKind kind;
        Buffer *buffer;
    } buffer;
};

class MaterialBase : public Object
{
    CLASS(MaterialBase, Object);

public:
    MaterialBase(const Ref<Shader>& shader)
        : m_shader(shader), m_param_changed(true)
    {
    }

    virtual ~MaterialBase() {}

    const Ref<Shader>& get_shader() const
    {
        return m_shader;
    }

    Ref<Shader>& get_shader()
    {
        return m_shader;
    }

    bool has_param_changed() const
    {
        return m_param_changed;
    }

    void set_param(const StringView& name, const Ref<Buffer>& buffer);
    void set_param(const StringView& name, const Ref<Texture>& texture);

    const MaterialParamCache& get_param(const StringView& name) const;

    void clear_param_changed();

private:
    Ref<Shader> m_shader;
    std::map<std::string, MaterialParamCache> m_caches;

    bool m_param_changed : 1;
};

enum class MaterialFlagBits
{
    None,
    Transparency = 1 << 0,
    DrawBeforeEverything = 1 << 1,
};
using MaterialFlags = Flags<MaterialFlagBits>;
DEFINE_FLAG_TRAITS(MaterialFlagBits);

class Material : public MaterialBase
{
    CLASS(Material, MaterialBase);

public:
    static Ref<Material> create(const Ref<Shader>& shader, std::optional<InstanceLayout> instance_layout = std::nullopt, MaterialFlags flags = MaterialFlagBits::None, PolygonMode polygon_mode = PolygonMode::Fill, CullMode cull_mode = CullMode::Back);

    Material(const Ref<Shader>& shader, std::optional<InstanceLayout> instance_layout, MaterialFlags flags, PolygonMode polygon_mode, CullMode cull_mode)
        : MaterialBase(shader), m_instance_layout(instance_layout), m_flags(flags), m_polygon_mode(polygon_mode), m_cull_mode(cull_mode)
    {
    }

    virtual ~Material() {}

    std::optional<InstanceLayout> get_instance_layout() const
    {
        return m_instance_layout;
    }

    PolygonMode get_polygon_mode() const
    {
        return m_polygon_mode;
    }

    CullMode get_cull_mode() const
    {
        return m_cull_mode;
    }

    MaterialFlags get_flags() const
    {
        return m_flags;
    }

private:
    std::optional<InstanceLayout> m_instance_layout;
    MaterialFlags m_flags;
    PolygonMode m_polygon_mode;
    CullMode m_cull_mode;
};

/**
 * @brief Type of material used for compute operations.
 */
class ComputeMaterial : public MaterialBase
{
    CLASS(ComputeMaterial, MaterialBase);

public:
    static Ref<ComputeMaterial> create(const Ref<Shader>& shader);

    ComputeMaterial(Ref<Shader> shader)
        : MaterialBase(shader)
    {
    }

    virtual ~ComputeMaterial() {}
};

struct RenderPassColorAttachment
{
    TextureFormat format = TextureFormat::RGBA8Srgb;

    /**
     * This attachment references the surface texture. If this is true then `format` is ignored.
     */
    bool surface_texture = false;

    bool operator<(const RenderPassColorAttachment& other) const
    {
        return format < other.format || surface_texture < other.surface_texture;
    }
};

struct RenderPassDepthAttachment
{
    bool save : 1 = false;
    bool load : 1 = false;

    bool operator<(const RenderPassDepthAttachment& other) const
    {
        return save < other.save || load < other.load;
    }
};

struct RenderPassDescriptor
{
    std::string name;
    std::vector<RenderPassColorAttachment> color_attachments = {};
    std::optional<RenderPassDepthAttachment> depth_attachment = {};

    bool operator<(const RenderPassDescriptor& other) const
    {
        return name < other.name;
    }
};

class RenderingDriver : public Object
{
    CLASS(RenderingDriver, Object);

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
        singleton = new T();
    };

    /**
     * @brief Returns the singleton for the rendering driver.
     */
    static RenderingDriver *get()
    {
        return singleton;
    }

    static void destroy_singleton()
    {
        delete singleton;
    }

    /**
     * @brief Initialize the underlaying graphics API.
     * @param window
     */
    [[nodiscard]]
    virtual Result<> initialize(const Window& window, bool enable_validation) = 0;

    /**
     * @brief Initialize ImGui. no-op when `__has_debug_menu`.
     */
    [[nodiscard]]
    virtual Result<> initialize_imgui(const Window& window) = 0;

    /**
     * @brief Configure the surface and swapchain.
     * @param window
     * @param vsync Enable or disable vsync for the surface.
     */
    [[nodiscard]]
    virtual Result<> configure_surface(const Window& window, VSync vsync) = 0;

    virtual void poll() = 0;

    /**
     * Limit the maximum number of frames per seconds. Set to `0` to remove the limit.
     * @param limit The limit of FPS.
     */
    virtual void limit_frames(uint32_t limit) = 0;

    /**
     * @brief Allocate a buffer in the GPU memory.
     * @param size Size of the buffer
     * @param flags
     * @param visibility
     */
    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer(const char *name, size_t size, BufferUsageFlags flags = BufferUsageFlagBits::None, BufferVisibility visibility = BufferVisibility::GPUOnly) = 0;

    /**
     * @brief Allocate a buffer in the GPU memory and fill it with `data`.
     * @param size Size of the buffer
     * @param data Data to upload to the buffer after creation. Must be the same size as `size`.
     * @param flags
     * @param visibility
     */
    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer_from_data(const char *name, size_t size, View<uint8_t> data, BufferUsageFlags flags = BufferUsageFlagBits::None, BufferVisibility visibility = BufferVisibility::GPUOnly);

    /**
     * @brief Create texture stored on the GPU.
     * @param width Pixel width of the texture
     * @param height Pixel height of the texture
     * @param format Format of the pixel
     * @param usage
     */
    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsageFlags usage, TextureDimension dimension = TextureDimension::D2D, uint32_t layers = 1) = 0;

    /**
     * @brief Draw a frame using a `RenderGraph`.
     */
    virtual void draw_graph(const RenderGraph& graph) = 0;

    /**
     * @brief Returns the size of the current rendering surface.
     */
    inline Extent2D get_surface_extent() const
    {
        return m_surface_extent;
    }

protected:
    Extent2D m_surface_extent;

private:
    static inline RenderingDriver *singleton = nullptr;
};

template <typename Value, typename Key>
class Cache
{
public:
    const Value& get(const Key& key) const
    {
        const auto& iter = m_objects.find(key);
        if (iter != m_objects.end())
        {
            return iter.second;
        }
        else
        {
            Value obj = create_object(key);
            m_objects[key] = obj;
            return m_objects[key];
        }
    }

    Value& get(const Key& key)
    {
        const auto& iter = m_objects.find(key);
        if (iter != m_objects.end())
        {
            return iter->second;
        }
        else
        {
            Value obj = create_object(key);
            m_objects[key] = obj;
            return m_objects[key];
        }
    }

protected:
    virtual Value create_object(const Key& key) = 0;

private:
    std::map<Key, Value> m_objects;
};
