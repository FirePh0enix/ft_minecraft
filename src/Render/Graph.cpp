#include "Render/Graph.hpp"

RenderPassEncoder::RenderPassEncoder(const RenderPassDescriptor& desc)
{
    g_graph.m_instructions.push_back(BeginRenderPassInstruction{.name = desc.name, .descriptor = desc});
}

RenderPassEncoder::~RenderPassEncoder()
{
    g_graph.m_instructions.push_back(EndRenderPassInstruction{});
}

void RenderPassEncoder::bind_material(const Ref<Material>& material)
{
    g_graph.m_instructions.push_back(BindMaterialInstruction{.material = material});
}

void RenderPassEncoder::bind_index_buffer(const Ref<Buffer>& buffer)
{
    g_graph.m_instructions.push_back(BindIndexBufferInstruction{.buffer = buffer});
}

void RenderPassEncoder::bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location)
{
    g_graph.m_instructions.push_back(BindVertexBufferInstruction{.buffer = buffer, .location = location});
}

void RenderPassEncoder::push_constants(const DataBuffer& buffer)
{
    g_graph.m_instructions.push_back(PushConstantsInstruction{.buffer = buffer});
}

void RenderPassEncoder::draw(uint32_t vertex_count, uint32_t instance_count)
{
    g_graph.m_instructions.push_back(DrawInstruction{.vertex_count = vertex_count, .instance_count = instance_count});
}

void RenderPassEncoder::imgui()
{
#ifdef __has_debug_menu
    g_graph.m_instructions.push_back(ImGuiDrawInstruction{});
#endif
}

void RenderPassEncoder::end()
{
    m_end = true;
    g_graph.m_instructions.push_back(EndRenderPassInstruction{});
}

ComputePassEncoder::ComputePassEncoder()
{
    // g_graph.m_compute_instructions.push_back(BeginComputePassInstruction{});
    g_graph.m_instructions.push_back(BeginComputePassInstruction{});
}

ComputePassEncoder::~ComputePassEncoder()
{
    if (!m_end)
        end();
}

void ComputePassEncoder::bind_material(const Ref<ComputeMaterial>& material)
{
    g_graph.m_instructions.push_back(BindMaterialInstruction{.material = material});
    // g_graph.m_compute_instructions.push_back(BindMaterialInstruction{.material = material});
}

void ComputePassEncoder::dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z)
{
    g_graph.m_instructions.push_back(DispatchInstruction{.group_x = group_x, .group_y = group_y, .group_z = group_z});
    // g_graph.m_compute_instructions.push_back(DispatchInstruction{.group_x = group_x, .group_y = group_y, .group_z = group_z});
}

void ComputePassEncoder::end()
{
    m_end = true;
    g_graph.m_instructions.push_back(EndComputePassInstruction{});
    // g_graph.m_compute_instructions.push_back(EndComputePassInstruction{});
}

RenderGraph& RenderGraph::get()
{
    return g_graph;
}

RenderGraph::RenderGraph()
{
}

void RenderGraph::reset()
{
    m_renderpass = false;
    m_instructions.clear();

    m_instructions.reserve(500);
}

View<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

View<Instruction> RenderGraph::get_compute_instructions() const
{
    return m_compute_instructions;
}

RenderPassEncoder RenderGraph::render_pass_begin(const RenderPassDescriptor& descriptor)
{
    return RenderPassEncoder(descriptor);
}

ComputePassEncoder RenderGraph::compute_pass_begin()
{
    return ComputePassEncoder();
}

void RenderGraph::copy_buffer(const Ref<Buffer>& dest, const Ref<Buffer>& source)
{
    m_instructions.push_back(CopyInstruction{.src = source, .dst = dest, .src_offset = 0, .dst_offset = 0, .size = dest->size_bytes()});
}
