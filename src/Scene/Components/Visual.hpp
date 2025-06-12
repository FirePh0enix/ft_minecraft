#pragma once

#include "Camera.hpp"
#include "Render/Graph.hpp"
#include "Scene/Components/Component.hpp"

class VisualComponent : public Component
{
    CLASS(VisualComponent, Component);

public:
    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) = 0;
};
