#pragma once

#include "Core/Types.hpp"
#include "Render/Renderer.hpp"
#include "UI/UI.hpp"

struct GPU_ATTRIBUTE TextureRectUniforms
{
    glm::mat4 model_matrix = glm::mat4();
};

class TextureRect : public UI
{
    CLASS(TextureRect, UI);

public:
    TextureRect();
    virtual ~TextureRect() {}

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPass& pass) override;

    void set_texture(const std::shared_ptr<Texture>& texture) { m_bg->set_param("image", texture); }

private:
    std::shared_ptr<Buffer> m_uniforms;
    std::shared_ptr<BindGroup> m_bg;
};
