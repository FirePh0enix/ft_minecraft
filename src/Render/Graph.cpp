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

View<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

void RenderGraph::begin_render_pass(RenderPassDescriptor descriptor)
{
    m_instructions.push_back(BeginRenderPassInstruction{.name = descriptor.name, .descriptor = descriptor});
    m_renderpass = true;
}

void RenderGraph::end_render_pass()
{
    ERR_COND(!m_renderpass, "Not inside a renderpass");
    m_instructions.push_back(EndRenderPassInstruction{});
    m_renderpass = false;
}

void RenderGraph::add_draw(Ref<Mesh> mesh, Ref<Material> material, glm::mat4 view_matrix, uint32_t instance_count, std::optional<Ref<Buffer>> instance_buffer, bool ignore_depth_prepass)
{
    if (instance_count == 0)
    {
        return;
    }

    ERR_COND(!m_renderpass, "Cannot draw outside of a renderpass");
    m_instructions.push_back(DrawInstruction{.mesh = mesh, .material = material, .instance_count = instance_count, .instance_buffer = instance_buffer, .view_matrix = view_matrix, .ignore_depth_prepass = ignore_depth_prepass});
}

void RenderGraph::add_copy(Ref<Buffer> src, Ref<Buffer> dst, size_t size, size_t src_offset, size_t dst_offset)
{
    ERR_COND(m_renderpass, "Cannot copy inside of a renderpass");
    m_instructions.push_back(CopyInstruction{.src = src, .dst = dst, .src_offset = src_offset, .dst_offset = dst_offset, .size = size});
}
