#pragma once

#include "Core/Containers/View.hpp"
#include "Render/Driver.hpp"

#include <variant>

struct BeginRenderPassInstruction
{
    String name;
    RenderPassDescriptor descriptor = {};
};

struct EndRenderPassInstruction
{
};

struct BeginComputePassInstruction
{
    String name;
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

using Instruction = std::variant<BeginRenderPassInstruction, EndRenderPassInstruction, BeginComputePassInstruction, EndComputePassInstruction, DrawInstruction, CopyInstruction, ImGuiDrawInstruction, BindIndexBufferInstruction, BindVertexBufferInstruction, BindMaterialInstruction, DispatchInstruction>;

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
    void draw(uint32_t vertex_count, uint32_t instance_count);
    void imgui();
    void end();

private:
    bool m_end = false;
};

class RenderGraph
{
public:
    friend class RenderPassEncoder;
    friend class ComputePassEncoder;

    RenderGraph();

    static RenderGraph& get();

    void reset();
    View<Instruction> get_instructions() const;

    RenderPassEncoder render_pass_begin(const RenderPassDescriptor& descriptor);
    void copy_buffer(const Ref<Buffer>& dest, const Ref<Buffer>& source);

private:
    Vector<Instruction> m_instructions;
};

static inline RenderGraph g_graph;
