#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Render/Renderer.hpp"

class RenderGraphNode : public Object
{
    CLASS(Node, Object);

    friend class RenderGraph;

public:
    virtual void begin(WGPUCommandEncoder encoder, WGPUTextureView surface_view) = 0;
    virtual void end() = 0;

    Ref<RenderGraphNode> next() const { return m_next; }
    void set_next(Ref<RenderGraphNode> next) { m_next = next; };

    bool is_final_pass() const { return m_final_pass; }

private:
    Ref<RenderGraphNode> m_next;
    bool m_final_pass;
};

class RenderPassNode : public RenderGraphNode
{
    CLASS(RenderPassNode, RenderGraphNode);

public:
    virtual void begin(WGPUCommandEncoder encoder, WGPUTextureView surface_view) override;
    virtual void end() override;

    WGPURenderPassEncoder encoder() const { return m_encoder; }
    Ref<Texture> get_color_output() const { return m_color_output; }
    Ref<Texture> get_depth_output() const { return m_depth_output; }

    void set_color_output(Ref<Texture> color_output) { m_color_output = color_output; }
    void set_depth_output(Ref<Texture> depth_output) { m_depth_output = depth_output; }

    void set_load_depth(bool v) { m_load_depth = v; };
    void set_output_to_surface(bool v) { m_output_to_surface = v; }

    bool output_to_surface() const { return m_output_to_surface; }

private:
    WGPURenderPassEncoder m_encoder = nullptr;
    Ref<Texture> m_color_output;
    Ref<Texture> m_depth_output;
    bool m_load_depth = false;
    bool m_output_to_surface = false;
};

class RenderGraph
{
public:
    WGPUCommandBuffer record(WGPUCommandEncoder encoder, WGPUTextureView surface_view, std::function<void(const RenderPassNode& node)> f);

    void set_root(Ref<RenderGraphNode> root);

private:
    Ref<RenderGraphNode> m_root;
};
