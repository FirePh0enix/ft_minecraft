#include "Render/Graph.hpp"
#include "Core/Error.hpp"

RenderPassEncoder::RenderPassEncoder(const RenderPassDescriptor& desc)
{
    g_graph.add_instruction(BeginRenderPassInstruction{.name = desc.name, .descriptor = desc});
}

RenderPassEncoder::~RenderPassEncoder()
{
    g_graph.add_instruction(EndRenderPassInstruction{});
}

void RenderPassEncoder::bind_material(const Ref<Material>& material)
{
    g_graph.add_instruction(BindMaterialInstruction{.material = material});
}

void RenderPassEncoder::bind_index_buffer(const Ref<Buffer>& buffer)
{
    g_graph.add_instruction(BindIndexBufferInstruction{.buffer = buffer});
}

void RenderPassEncoder::bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location)
{
    g_graph.add_instruction(BindVertexBufferInstruction{.buffer = buffer, .location = location});
}

void RenderPassEncoder::push_constants(const DataBuffer& buffer)
{
    g_graph.add_instruction(PushConstantsInstruction{.buffer = buffer});
}

void RenderPassEncoder::draw(uint32_t vertex_count, uint32_t instance_count)
{
    g_graph.add_instruction(DrawInstruction{.vertex_count = vertex_count, .instance_count = instance_count});
}

void RenderPassEncoder::end()
{
    m_end = true;
}

ComputePassEncoder::ComputePassEncoder()
{
    g_graph.add_instruction(BeginComputePassInstruction{});
}

ComputePassEncoder::~ComputePassEncoder()
{
    if (!m_end)
        g_graph.add_instruction(EndComputePassInstruction{});
}

void ComputePassEncoder::bind_material(const Ref<ComputeMaterial>& material)
{
    g_graph.add_instruction(BindMaterialInstruction{.material = material});
}

void ComputePassEncoder::dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z)
{
    g_graph.add_instruction(DispatchInstruction{.group_x = group_x, .group_y = group_y, .group_z = group_z});
}

void ComputePassEncoder::end()
{
    m_end = true;
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
    ZoneScoped;

    m_renderpass = false;
    m_instructions.clear();

    m_instructions.reserve(500);
}

View<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

void RenderGraph::add_instruction(const Instruction& inst)
{
    m_instructions.push_back(inst);
}

RenderPassEncoder RenderGraph::render_pass_begin(const RenderPassDescriptor& descriptor)
{
    return RenderPassEncoder(descriptor);
}

ComputePassEncoder RenderGraph::compute_pass_begin()
{
    return ComputePassEncoder();
}
