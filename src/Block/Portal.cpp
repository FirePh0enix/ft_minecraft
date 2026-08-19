#include "Block/Portal.hpp"

#include "Render/Renderer.hpp"

PortalBlock::PortalBlock()
{
    m_unbreakable = true;
    m_solid = false;
    m_mat = Material::create(Renderer::get().get_portal_shader(), MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV | MaterialFlagBits::StencilMask, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);
    m_mesh = Renderer::get().get_cube_mesh();
}

void PortalBlock::draw(const RenderPass& pass, glm::i64vec3 position)
{
    std::shared_ptr<Buffer> buf = EXPECT(Buffer::create(sizeof(FwModel), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    FwModel model{};
    model.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(position)) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0, 1.0, 0.5));
    buf->update_struct(model);

    // std::shared_ptr<Buffer> buf = EXPECT(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    // FwColored model{};
    // model.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(position)); // * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0, 1.0, 0.5));
    // model.color = Colors::yellow;
    // buf->update_struct(model);

    std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_portal_shader());
    bg->set_param("world_env", Renderer::get().get_fw_world_env());
    bg->set_param("camera", Renderer::get().get_fw_camera());
    bg->set_param("model", buf);

    Renderer::get().draw(pass, m_mesh, m_mat, bg, nullptr, 1, 0x2);
}
