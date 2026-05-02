#include "Render/Graph.hpp"
#include "Core/Assert.hpp"

void RenderPassNode::begin(WGPUCommandEncoder encoder, WGPUTextureView surface_view)
{
    WGPURenderPassDescriptor desc{};
    desc.nextInChain = nullptr;
    desc.label = WGPU_STRING_VIEW_INIT;
    desc.timestampWrites = nullptr;
    desc.occlusionQuerySet = nullptr;

    InplaceVector<WGPURenderPassColorAttachment, 4> color_attachs;
    WGPURenderPassDepthStencilAttachment depth_attach{};

    if (!m_color_output.is_null())
    {
        WGPURenderPassColorAttachment attach{};
        attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 1.0);
        attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        attach.loadOp = WGPULoadOp_Clear;
        attach.storeOp = WGPUStoreOp_Store;
        attach.view = m_output_to_surface ? surface_view : m_color_output->handle_view();

        color_attachs.push_back(attach);
    }

    desc.colorAttachmentCount = color_attachs.size();
    desc.colorAttachments = color_attachs.data();

    if (!m_depth_output.is_null())
    {
        depth_attach.depthClearValue = 1.0;
        depth_attach.depthLoadOp = m_load_depth ? WGPULoadOp_Load : WGPULoadOp_Clear;
        depth_attach.depthStoreOp = WGPUStoreOp_Store; // TODO: discard if not needed WGPUStoreOp_Discard (like at the end of the graph)
        depth_attach.view = m_depth_output->handle_view();

        desc.depthStencilAttachment = &depth_attach;
    }

    m_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &desc);
}

void RenderPassNode::end()
{
    wgpuRenderPassEncoderEnd(m_encoder);
    wgpuRenderPassEncoderRelease(m_encoder);
}

WGPUCommandBuffer RenderGraph::record(WGPUCommandEncoder encoder, WGPUTextureView surface_view, std::function<void(const RenderPassNode& node)> f)
{
    Ref<Node> node = m_root;
    while (!node.is_null())
    {
        if (Ref<RenderPassNode> render_pass_node = node)
        {
            node->begin(encoder, surface_view);
            f(*node.cast_to<RenderPassNode>());
            node->end();
        }

        node = node->next();
    }

    return wgpuCommandEncoderFinish(encoder, nullptr);
}

void RenderGraph::set_root(Ref<Node> root)
{
    Ref<Node> node = root;
    while (!node->next().is_null())
    {
        node->m_final_pass = false;
        node = node->next();
    }

    ASSERT(node->is<RenderPassNode>(), "");

    node->m_final_pass = true;
    m_root = root;
}
