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

void ColorRect::draw(WGPURenderPassEncoder encoder)
{
    m_uniforms->update(View(ColorRectUniforms(matrix(), m_color)).as_bytes());

    const Ref<Mesh>& mesh = Renderer::get().get_square_mesh();

    WGPURenderPipeline pipeline = Renderer::get().get_pipeline(m_material, Renderer::get().get_surface_format());
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_material->get_bind_group(), 0, nullptr);

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());

    wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
}
