#include "Render/Graph.hpp"
#include "Core/Error.hpp"

RenderGraph::RenderGraph()
{
}

void RenderGraph::reset()
{
    ZoneScoped;

    m_renderpass = false;
    m_instructions.clear();
}

Span<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

void RenderGraph::begin_render_pass()
{
    m_instructions.push_back({.renderpass = {.kind = InstructionKind::BeginRenderPass}});
    m_renderpass = true;
}

void RenderGraph::end_render_pass()
{
    ERR_COND(!m_renderpass, "Not inside a renderpass");
    m_instructions.push_back({.kind = InstructionKind::EndRenderPass});
    m_renderpass = false;
}

void RenderGraph::add_draw(Mesh *mesh, Material *material, glm::mat4 view_matrix, uint32_t instance_count, std::optional<Buffer *> instance_buffer)
{
    if (instance_count == 0)
    {
        return;
    }

    ERR_COND(!m_renderpass, "Cannot draw outside of a renderpass");
    m_instructions.push_back({.draw = {.kind = InstructionKind::Draw, .mesh = mesh, .material = material, .instance_count = instance_count, .instance_buffer = instance_buffer, .view_matrix = view_matrix}});
}

void RenderGraph::add_copy(Buffer *src, Buffer *dst, size_t size, size_t src_offset, size_t dst_offset)
{
    ERR_COND(m_renderpass, "Cannot copy inside of a renderpass");
    m_instructions.push_back({.copy = {.kind = InstructionKind::Copy, .src = src, .dst = dst, .src_offset = src_offset, .dst_offset = dst_offset, .size = size}});
}
