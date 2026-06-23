#include "Entity/Arrow.hpp"

#include "Core/Print.hpp"
#include "Core/Types.hpp"
#include "Engine.hpp"
#include "Entity/Mob.hpp"
#include "Render/Renderer.hpp"
#include "glm/ext/vector_float3.hpp"

struct GPU_ATTRIBUTE ItemBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

ArrowEntity::ArrowEntity(Id<Item> item) : m_dir(glm::vec3()), m_item(item)
{
    m_aabb = AABB(-glm::vec3(0.2, 0.2, 0.2), glm::vec3(0.2, 0.2, 0.2));
    Ref<Block> block = Engine::get().registry().block_from_item(item);
    m_textures = glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16));
}

void ArrowEntity::tick(float delta)
{
    m_velocity += m_dir * 3.0f * delta;
    m_velocity.y -= m_gravity_value * delta;

    move_and_collide();


    auto entities = m_world->get_dimension(m_dimension).cast_box(m_aabb);
    for (Ref<Entity>& e : entities)
    {

        println("Arrow hit something!");

        if (auto mob = e.cast_to<Mob>())
            mob->damage(1, id());

        m_velocity = glm::vec3(0.0f);
        m_active = false;
        break;
    }
}

void ArrowEntity::draw(const RenderPass& pass)
{
    ItemBlockModel matrix(get_transform().to_matrix(), m_textures);
    m_model_buffer->update(View(matrix).as_bytes());
    Renderer::get().draw(pass, Renderer::get().get_cube_mesh(), m_material);
}
