#include "UI/TextureRect.hpp"

TextureRect::TextureRect()
{
    m_uniforms = EXPECT(Buffer::create(sizeof(TextureRectUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_bg = BindGroup::create(Renderer::get().get_texture_rect_shader());
    m_bg->set_param("env", Renderer::get().get_env_2d());
    m_bg->set_param("uniforms", m_uniforms);
}

void TextureRect::update(float d)
{
    (void)d;
}

void TextureRect::process_event(Event& event)
{
    if (event.handled)
        return;
}

void TextureRect::draw(const RenderPass& pass)
{
    m_uniforms->update(View(TextureRectUniforms(matrix())).as_bytes());
    Renderer::get().draw(pass, Renderer::get().get_square_mesh(), Renderer::get().get_fw_texture_rect_mat(), m_bg);
}
