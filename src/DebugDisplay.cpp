#include "DebugDisplay.hpp"

#include "Engine.hpp"
#include "Render/Renderer.hpp"

DebugCube::DebugCube(glm::dvec3 position, glm::vec3 scale, Color color, float duration, float creation_time)
    : color(color)
{
    this->position = position;
    this->scale = scale;

    this->duration = duration;
    this->creation_time = creation_time;

    buffer = EXPECT(Buffer::create(sizeof(FwColored), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));

    bg = BindGroup::create(Renderer::get().get_fw_colored_shader());
    bg->set_param("world_env", Renderer::get().get_fw_world_env());
    bg->set_param("camera", Renderer::get().get_fw_camera());
    bg->set_param("model", buffer);
}

void DebugCube::draw(const RenderPass& pass) const
{
    FwColored colored{};
    colored.color = color;
    colored.model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(position - Engine::get().get_world()->get_player()->get_camera()->get_global_transform().position()));
    buffer->update_struct(colored);

    Renderer::get().draw(pass, Renderer::get().get_wireframe_cube_mesh(), Renderer::get().get_wireframe_mat(), bg);
}

DebugDisplay::DebugDisplay()
{
}

void DebugDisplay::update(float delta)
{
    m_timer += delta;

    std::vector<uint64_t> to_remove;
    for (const auto& [id, shape] : m_shapes)
    {
        if (m_timer - shape->creation_time >= shape->duration)
            to_remove.push_back(id);
    }
    for (uint64_t id : to_remove)
    {
        m_shapes.erase(id);
    }
}

void DebugDisplay::draw(const RenderPass& pass)
{
    for (const auto& [id, iter] : m_shapes)
        iter->draw(pass);
}

void DebugDisplay::draw_cube(glm::dvec3 position, glm::vec3 size, Color color, float duration)
{
    m_shapes[m_id++] = std::make_unique<DebugCube>(position, size, color, duration, m_timer);
}
