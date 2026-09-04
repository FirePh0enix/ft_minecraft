#include "Entity/Item.hpp"

#include "Core/Types.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"

ItemEntity::ItemEntity(Id<Item> item)
    : m_item(item), m_time(0)
{
    m_aabb = AABBd(-glm::vec3(0.2, 0.2, 0.2), glm::vec3(0.2, 0.2, 0.2));
    get_transform().scale() = glm::dvec3(0.2, 0.2, 0.2);

    m_model_buffer = EXPECT(Buffer::create(sizeof(FwModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    // TODO: create the mesh only on time per block not each time I create a new entity.
    m_bg = BindGroup::create(Renderer::get().get_model_noshadow_shader());
    m_bg->set_param("camera", Renderer::get().get_fw_camera());
    m_bg->set_param("model", m_model_buffer);
    m_bg->set_param("world_env", Renderer::get().get_fw_world_env());
    m_bg->set_param("atlas", EXPECT(Engine::get().registry().get_atlas()->get_view()));

    std::shared_ptr<Block> block = Engine::get().registry().block_from_item(item);

    MeshBuilder builder;
    block->add(builder, {}, {});
    m_mesh = EXPECT(builder.build());
}

void ItemEntity::tick(float delta)
{
    m_velocity.y -= m_gravity_value * delta;
    move_and_collide();
    m_time += delta;
}

void ItemEntity::draw(const RenderPass& pass)
{
    m_transform.rotation() = glm::rotate(glm::identity<glm::quat>(), m_time, glm::vec3(0.0, 1.0, 0.0));

    FwModel matrix(get_transform().to_matrix(Engine::get().get_world()->get_player()->get_position()));

    m_model_buffer->update_struct(matrix);
    Renderer::get().draw(pass, m_mesh, Renderer::get().get_model_noshadow_mat(), m_bg);
}
