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

using Instruction = std::variant<BeginRenderPassInstruction, EndRenderPassInstruction, DrawInstruction, CopyInstruction, ImGuiDrawInstruction>;

struct PushConstants
{
    glm::mat4 view_matrix;
    // NaN does not exists with WGSL.
    glm::float32 nan;
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

private:
    std::vector<CopyInstruction> m_copy_instructions;
    std::vector<Instruction> m_instructions;
    bool m_renderpass = false;
};

static inline RenderGraph g_graph;
