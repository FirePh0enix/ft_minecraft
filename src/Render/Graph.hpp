#pragma once

#include "Core/Containers/View.hpp"
#include "Core/DataBuffer.hpp"
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

struct BeginComputePassInstruction
{
    std::string name;
};

struct EndComputePassInstruction
{
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
    Ref<MaterialBase> material = nullptr;
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

struct DrawInstruction
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
    DataBuffer buffer;
};

using Instruction = std::variant<BeginRenderPassInstruction, EndRenderPassInstruction, BeginComputePassInstruction, EndComputePassInstruction, DrawInstruction, CopyInstruction, ImGuiDrawInstruction, BindIndexBufferInstruction, BindVertexBufferInstruction, BindMaterialInstruction, PushConstantsInstruction, DispatchInstruction>;

class RenderPassEncoder
{
public:
    RenderPassEncoder(const RenderPassDescriptor& desc);
    ~RenderPassEncoder();

    RenderPassEncoder(const RenderPassEncoder& encoder) = delete;
    RenderPassEncoder operator==(const RenderPassEncoder& encoder) = delete;

    void bind_material(const Ref<Material>& material);
    void bind_index_buffer(const Ref<Buffer>& buffer);
    void bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location);
    void push_constants(const DataBuffer& buffer);
    void draw(uint32_t vertex_count, uint32_t instance_count);
    void end();

private:
    bool m_end = false;
};

class ComputePassEncoder
{
public:
    ComputePassEncoder();
    ~ComputePassEncoder();

    ComputePassEncoder(const ComputePassEncoder& encoder) = delete;
    ComputePassEncoder operator==(const ComputePassEncoder& encoder) = delete;

    void bind_material(const Ref<ComputeMaterial>& material);
    void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z);
    void end();

private:
    bool m_end = false;
};

class RenderGraph
{
public:
    RenderGraph();

    static RenderGraph& get();

    void reset();
    View<Instruction> get_instructions() const;

    void add_instruction(const Instruction& inst);

    RenderPassEncoder render_pass_begin(const RenderPassDescriptor& descriptor);
    ComputePassEncoder compute_pass_begin();

    /**
     * @brief Start a render pass.
     * @param descriptor Structure to describe the render pass.
     */
    // void begin_render_pass(RenderPassDescriptor descriptor);

    // /**
    //  * @brief End the current render pass.
    //  */
    // void end_render_pass();

    // void begin_compute_pass();
    // void end_compute_pass();

    /**
     * @brief Add imgui draw calls. no-op when `__has_debug_menu` is not set.
     */
    // void add_imgui_draw();

    // void bind_material(const Ref<MaterialBase>& material);

    // void bind_index_buffer(const Ref<Buffer>& buffer);

    // void bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location);

    // void push_constants(const DataBuffer& buffer);

    // void draw(uint32_t vertex_count, uint32_t instance_count);

    // void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z);

private:
    std::vector<Instruction> m_instructions;
    bool m_renderpass = false;
};

static inline RenderGraph g_graph;
