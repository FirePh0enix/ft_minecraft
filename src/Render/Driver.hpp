#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/View.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Shader;
class RenderGraph;

class Buffer : public Object
{
    CLASS(Buffer, Object);

public:
    /**
     * @brief Update the content of the buffer.
     */
    virtual void update(View<uint8_t> view, size_t offset = 0) = 0;

    inline size_t size() const
    {
        return m_size;
    }

    virtual ~Buffer() {}

protected:
    size_t m_size;
};

class Texture : public Object
{
    CLASS(Texture, Object);

public:
    /**
     * @brief Update the content of a layer of the texture.
     */
    virtual void update(View<uint8_t> view, uint32_t layer = 0) = 0;

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

class Mesh : public Object
{
    CLASS(Mesh, Object);

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

struct MaterialFlags
{
    /**
     * Enable transparent blending for the material.
     */
    bool transparency : 1 = false;

    /**
     * Workaround to always draw the material before everything else.
     * Can be used for skyboxes.
     */
    bool always_first : 1 = false;
};

/**
 * @brief Describe the layout of a material which can be used to create multiple materials.
 *
 * This contains information about a type of material, allowing the reuse of information to create multiple
 * materials derived from it.
 */
class MaterialLayout : public Object
{
    CLASS(MaterialLayout, Object);

public:
    virtual ~MaterialLayout() {}
};

class Material : public Object
{
    CLASS(Material, Object);

public:
    virtual void set_param(const std::string& name, const Ref<Buffer>& buffer) = 0;
    virtual void set_param(const std::string& name, const Ref<Texture>& texture) = 0;

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
    virtual Result<> initialize(const Window& window) = 0;

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
     */
    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer(size_t size, BufferUsage flags = {}, BufferVisibility visibility = BufferVisibility::GPUOnly) = 0;

    /**
     * @brief Allocate a buffer in the GPU memory and fill it with `data`.
     * @param size Size of the buffer
     * @param data Data to upload to the buffer after creation. Must be the same size as `size`.
     * @param flags
     * @param visibility
     */
    [[nodiscard]]
    virtual Result<Ref<Buffer>> create_buffer_from_data(size_t size, View<uint8_t> data, BufferUsage flags = {}, BufferVisibility visibility = BufferVisibility::GPUOnly);

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) = 0;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers) = 0;

    [[nodiscard]]
    virtual Result<Ref<Texture>> create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage) = 0;

    /**
     * @brief Create a mesh.
     * @param index_type Type of indices used for this mesh.
     * @param indices Indices of the mesh. Must be an array of `uint32_t` if index_type is `IndexType::U32` or `uint16_t` if index_type is `IndexType::U16`.
     * @param vertices Vertices of the mesh.
     * @param uvs Texture coordinates (UV) of the mesh.
     * @param normals Normals of the mesh.
     */
    [[nodiscard]]
    virtual Result<Ref<Mesh>> create_mesh(IndexType index_type, View<uint8_t> indices, View<glm::vec3> vertices, View<glm::vec2> uvs, View<glm::vec3> normals) = 0;

    [[nodiscard]]
    virtual Result<Ref<MaterialLayout>> create_material_layout(Ref<Shader> shader, MaterialFlags flags = {}, std::optional<InstanceLayout> instance_layout = std::nullopt, CullMode cull_mode = CullMode::Back, PolygonMode polygon_mode = PolygonMode::Fill) = 0;

    [[nodiscard]]
    virtual Result<Ref<Material>> create_material(const Ref<MaterialLayout>& layout) = 0;

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
    static inline RenderingDriver *singleton = nullptr;
};
