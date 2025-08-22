#pragma once

#include "Core/Containers/View.hpp"
#include "Render/Driver.hpp"

#include <variant>

class Buffer;
class Mesh;
class Material;

struct BeginRenderPassInstruction
{
    std::string name;
    RenderPassDescriptor descriptor = {};
};

struct EndRenderPassInstruction
{
};

struct DrawInstruction
{
    Ref<Mesh> mesh = nullptr;
    Ref<Material> material = nullptr;
    size_t instance_count = 0;
    std::optional<Ref<Buffer>> instance_buffer = std::nullopt;
    glm::mat4 view_matrix = glm::mat4(1.0);
};

struct CopyInstruction
{
    Ref<Buffer> src = nullptr;
    Ref<Buffer> dst = nullptr;
    size_t src_offset = 0;
    size_t dst_offset = 0;
    size_t size = 0;
};

struct ImGuiDrawInstruction
{
};

struct BindMaterialInstruction
{
    Ref<Material> material = nullptr;
};

struct BindIndexBufferInstruction
{
    Ref<Buffer> buffer = nullptr;
    IndexType index_type = IndexType::Uint16;
};

struct BindVertexBufferInstruction
{
    Ref<Buffer> buffer = nullptr;
    uint32_t location = 0;
};

struct Draw2Instruction
{
    uint32_t vertex_count;
    uint32_t instance_count;
};

struct DispatchInstruction
{
    uint32_t group_x = 0;
    uint32_t group_y = 0;
    uint32_t group_z = 0;
};

struct PushConstantsInstruction
{
    std::vector<char> buffer;
};

using Instruction = std::variant<BeginRenderPassInstruction, EndRenderPassInstruction, DrawInstruction, CopyInstruction, ImGuiDrawInstruction, BindIndexBufferInstruction, BindVertexBufferInstruction, BindMaterialInstruction, PushConstantsInstruction, Draw2Instruction, DispatchInstruction>;

struct PushConstants
{
    glm::mat4 view_matrix;
};

class RenderGraph
{
public:
    RenderGraph();

    static RenderGraph& get();

    void reset();
    View<Instruction> get_instructions() const;
    View<CopyInstruction> get_copy_instructions() const;

    /**
     * @brief Start a render pass.
     * @param descriptor Structure to describe the render pass.
     */
    void begin_render_pass(RenderPassDescriptor descriptor);

    /**
     * @brief End the current render pass.
     */
    void end_render_pass();

    /**
     * @brief Add a draw call the the render pass.
     * @param mesh
     * @param material
     * @param view_matrix
     * @param instance_count Number of instance to draw. If this number is > 1, then `instance_buffer` must be present.
     * @param instance_buffer Data used for instancing.
     * @param ignore_depth_prepass
     */
    void add_draw(const Ref<Mesh>& mesh, const Ref<Material>& material, glm::mat4 view_matrix = {}, uint32_t instance_count = 1, std::optional<Ref<Buffer>> instance_buffer = {});

    void add_copy(const Ref<Buffer>& src, const Ref<Buffer>& dst, size_t size, size_t src_offset = 0, size_t dst_offset = 0);

    /**
     * @brief Add imgui draw calls. no-op when `__has_debug_menu` is not set.
     */
    void add_imgui_draw();

    void bind_material(const Ref<Material>& material);

    void bind_index_buffer(const Ref<Buffer>& buffer);

    void bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location);

    void push_constants(const std::vector<char>& buffer);

    void draw(uint32_t vertex_count, uint32_t instance_count);

    void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z);

private:
    std::vector<CopyInstruction> m_copy_instructions;
    std::vector<Instruction> m_instructions;
    bool m_renderpass = false;
};

static inline RenderGraph g_graph;
