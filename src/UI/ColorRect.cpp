#include "UI/ColorRect.hpp"
#include "Color.hpp"

ColorRect::ColorRect()
{
    m_uniforms = EXPECT(Buffer::create(sizeof(ColorRectUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_material = EXPECT(Material::create(Renderer::get().get_color_rect_shader(), MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_None, UVType::UV));
    m_material->set_param("env", Renderer::get().get_env_2d());
    m_material->set_param("uniforms", m_uniforms);

    set_color(Color());
}

void ColorRect::update(float d)
{
    (void)d;
}

void ColorRect::process_event(Event& event)
{
    if (event.handled)
        return;
}

void ColorRect::draw(const RenderPass& pass)
{
    m_uniforms->update(View(ColorRectUniforms(matrix(), m_color)).as_bytes());
    Renderer::get().draw(pass, Renderer::get().get_square_mesh(), m_material);
}
