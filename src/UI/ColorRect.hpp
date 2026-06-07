#pragma once

#include "Color.hpp"
#include "Core/Types.hpp"
#include "Render/Renderer.hpp"
#include "UI/UI.hpp"

struct GPU_ATTRIBUTE ColorRectUniforms
{
    glm::mat4 model_matrix = glm::mat4();
    Color color;
};

class ColorRect : public UI
{
    CLASS(ColorRect, UI);

public:
    ColorRect();
    virtual ~ColorRect() {}

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(WGPURenderPassEncoder encoder) override;

    void set_color(Color color) { m_color = color; }
    Color get_color() const { return m_color; }

private:
    Color m_color;
    Ref<Buffer> m_uniforms;
    Ref<Material> m_material;
};
