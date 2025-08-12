#include "Render/Graph.hpp"
#include "Core/Error.hpp"

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
    m_copy_instructions.clear();

    m_instructions.reserve(500);
}

View<Instruction> RenderGraph::get_instructions() const
{
    return m_instructions;
}

View<CopyInstruction> RenderGraph::get_copy_instructions() const
{
    return m_copy_instructions;
}

void RenderGraph::begin_render_pass(RenderPassDescriptor descriptor)
{
    ZoneScoped;

    m_instructions.push_back(BeginRenderPassInstruction{.name = descriptor.name, .descriptor = descriptor});
    m_renderpass = true;
}

void RenderGraph::end_render_pass()
{
    ZoneScoped;

    ERR_COND(!m_renderpass, "Not inside a renderpass");
    m_instructions.push_back(EndRenderPassInstruction{});
    m_renderpass = false;
}

void RenderGraph::add_draw(const Ref<Mesh>& mesh, const Ref<Material>& material, glm::mat4 view_matrix, uint32_t instance_count, std::optional<Ref<Buffer>> instance_buffer)
{
    ZoneScoped;

    if (instance_count == 0)
    {
        return;
    }

    ERR_COND(!m_renderpass, "Cannot draw outside of a renderpass");
    m_instructions.push_back(DrawInstruction{.mesh = mesh, .material = material, .instance_count = instance_count, .instance_buffer = instance_buffer, .view_matrix = view_matrix});
}

void RenderGraph::add_copy(const Ref<Buffer>& src, const Ref<Buffer>& dst, size_t size, size_t src_offset, size_t dst_offset)
{
    m_copy_instructions.push_back(CopyInstruction{.src = src, .dst = dst, .src_offset = src_offset, .dst_offset = dst_offset, .size = size});
}

void RenderGraph::add_imgui_draw()
{
#ifdef __has_debug_menu
    m_instructions.push_back(ImGuiDrawInstruction{});
#endif
}

void RenderGraph::bind_material(const Ref<Material>& material)
{
    m_instructions.push_back(BindMaterialInstruction{.material = material});
}

void RenderGraph::bind_index_buffer(const Ref<Buffer>& buffer)
{
    m_instructions.push_back(BindIndexBufferInstruction{.buffer = buffer});
}

void RenderGraph::bind_vertex_buffer(const Ref<Buffer>& buffer, uint32_t location)
{
    m_instructions.push_back(BindVertexBufferInstruction{.buffer = buffer, .location = location});
}

void RenderGraph::dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z)
{
    m_instructions.push_back(DispatchInstruction{.group_x = group_x, .group_y = group_y, .group_z = group_z});
}
