#include "UI/TextureRect.hpp"

TextureRect::TextureRect()
{
    m_uniforms = EXPECT(Buffer::create(sizeof(TextureRectUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_material = EXPECT(Material::create(Renderer::get().get_texture_rect_shader(), MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal, WGPUCullMode_None, UVType::UV));
    m_material->set_param("env", Renderer::get().get_env_2d());
    m_material->set_param("uniforms", m_uniforms);
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

void TextureRect::draw(WGPURenderPassEncoder encoder)
{
    m_uniforms->update(View(TextureRectUniforms(matrix())).as_bytes());

    const Ref<Mesh>& mesh = Renderer::get().get_square_mesh();

    WGPURenderPipeline pipeline = Renderer::get().get_pipeline(m_material, Renderer::get().get_surface_format());
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_material->get_bind_group(), 0, nullptr);

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, mesh->get_buffer(Mesh::BufferKind::UV)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::UV)->size());

    wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), 1, 0, 0, 0);
}
