#pragma once

#include "Core/Class.hpp"
#include "Core/Error.hpp"
#include "Core/Ref.hpp"
#include "Core/Span.hpp"
#include "Render/Graph.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "Window.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Shader;

class Buffer : public Object
{
    CLASS(Buffer, Object);

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

class Texture : public Object
{
    CLASS(Texture, Object);

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

    InstanceLayout(Span<InstanceLayoutInput> inputs, size_t stride)
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
     */
    [[nodiscard]]
    virtual Expected<void> initialize(const Window& window) = 0;

    /**
     * @brief Configure the surface and swapchain.
     * It must be called every time the window is resized.
     */
    [[nodiscard]]
    virtual Expected<void> configure_surface(const Window& window, VSync vsync) = 0;

    virtual void poll() = 0;

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
    virtual Expected<Ref<MaterialLayout>> create_material_layout(Ref<Shader> shader, MaterialFlags flags = {}, std::optional<InstanceLayout> instance_layout = std::nullopt, CullMode cull_mode = CullMode::Back, PolygonMode polygon_mode = PolygonMode::Fill) = 0;

    [[nodiscard]]
    virtual Expected<Ref<Material>> create_material(const Ref<MaterialLayout>& layout) = 0;

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
