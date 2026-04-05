#include "Render/Graph.hpp"

RenderPassEncoder::RenderPassEncoder(const RenderPassDescriptor& desc)
{
    g_graph.m_instructions.append(BeginRenderPassInstruction{.name = desc.name, .descriptor = desc});
}

RenderPassEncoder::~RenderPassEncoder()
{
    g_graph.m_instructions.append(EndRenderPassInstruction{});
}

void RenderPassEncoder::bind_material(const Ref<Material>& material)
{
    g_graph.m_instructions.append(BindMaterialInstruction{.material = material});
}

void RenderPassEncoder::bind_index_buffer(const Ref<Buffer>& buffer)
{
    g_graph.m_instructions.append(BindIndexBufferInstruction{.buffer = buffer});
}

void RenderPassEncoder::bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location)
{
    g_graph.m_instructions.append(BindVertexBufferInstruction{.buffer = buffer, .location = location});
}

void RenderPassEncoder::draw(uint32_t vertex_count, uint32_t instance_count)
{
    g_graph.m_instructions.append(DrawInstruction{.vertex_count = vertex_count, .instance_count = instance_count});
}

void RenderPassEncoder::imgui()
{
#ifdef __has_debug_menu
    g_graph.m_instructions.append(ImGuiDrawInstruction{});
#endif
}

void RenderPassEncoder::end()
{
    m_end = true;
    g_graph.m_instructions.append(EndRenderPassInstruction{});
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
    m_instructions.clear_keep_capacity();
}

View<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

RenderPassEncoder RenderGraph::render_pass_begin(const RenderPassDescriptor& descriptor)
{
    return RenderPassEncoder(descriptor);
}

void RenderGraph::copy_buffer(const Ref<Buffer>& dest, const Ref<Buffer>& source)
{
    m_instructions.append(CopyInstruction{.src = source, .dst = dest, .src_offset = 0, .dst_offset = 0, .size = dest->size_bytes()});
}
