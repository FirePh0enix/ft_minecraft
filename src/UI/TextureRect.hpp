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
    virtual void draw(WGPURenderPassEncoder encoder) override;

    void set_texture(Ref<Texture> texture) { m_material->set_param("image", texture); }

private:
    Ref<Buffer> m_uniforms;
    Ref<Material> m_material;
};
