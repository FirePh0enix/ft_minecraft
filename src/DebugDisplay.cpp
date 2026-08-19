#include "DebugDisplay.hpp"

#include "Render/Renderer.hpp"

DebugCube::DebugCube(glm::mat4 model, Color color, float duration, float creation_time)
{
    this->duration = duration;
    this->creation_time = creation_time;

    buffer = EXPECT(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    FwColored colored{};
    colored.color = color;
    colored.model = model;
    buffer->update_struct(colored);

    bg = BindGroup::create(Renderer::get().get_fw_colored_shader());
    bg->set_param("world_env", Renderer::get().get_fw_world_env());
    bg->set_param("camera", Renderer::get().get_fw_camera());
    bg->set_param("model", buffer);
}

void DebugCube::draw(const RenderPass& pass) const
{
    Renderer::get().draw(pass, Renderer::get().get_wireframe_cube_mesh(), Renderer::get().get_wireframe_mat(), bg);
}

DebugDisplay::DebugDisplay()
{
}

void DebugDisplay::update(float delta)
{
    m_timer += delta;

    for (size_t i = 0; i < m_shapes.size(); i++)
    {
        const auto& shape = m_shapes[i];
        if (m_timer - shape->creation_time >= shape->duration)
        {
            m_shapes.erase(m_shapes.begin() + (ssize_t)i);
            i--;
        }
    }
}

void DebugDisplay::draw(const RenderPass& pass)
{
    for (const auto& iter : m_shapes)
        iter->draw(pass);
}

void DebugDisplay::draw_cube(glm::vec3 position, glm::vec3 size, Color color, float duration)
{
    m_shapes.push_back(std::make_unique<DebugCube>(glm::translate(glm::identity<glm::mat4>(), position) * glm::scale(glm::identity<glm::mat4>(), size), color, duration, m_timer));
}
