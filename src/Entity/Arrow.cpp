#include "Entity/Arrow.hpp"

#include "Core/Print.hpp"
#include "Engine.hpp"
#include "Entity/Mob.hpp"
#include "Render/Renderer.hpp"
#include "glm/ext/vector_float3.hpp"
#include <memory>

ArrowEntity::ArrowEntity(Id<Item> item) : m_item(item)
{
    m_aabb = AABB(-glm::vec3(0.04f, 0.04f, 0.375f), glm::vec3(0.04f, 0.04f, 0.375f));
    std::shared_ptr<Block> block = Engine::get().registry().block_from_item(item);
    m_textures = glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16));
}

void ArrowEntity::on_ready()
{
    m_model = EXPECT(Model::load("assets/models/arrow.json"));
}

void ArrowEntity::tick(float delta)
{
    if (m_on_ground)
        return;

    m_velocity.y -= m_gravity_value * delta;

    // Rotate while in air.
    if (glm::length2(m_velocity) > 1e-8f)
    {
        const glm::vec3 direction = glm::normalize(m_velocity);
        get_transform().rotation() = glm::quatLookAt(-direction, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    move_and_collide();

    AABB search_box = AABB::from_center_extent(get_global_transform().position(), glm::vec3(0.2f));

    auto entities = m_world->get_dimension(m_dimension).cast_box(search_box);
    for (const auto& e : entities)
    {
        if (e->id() == this->id())
            continue;

        if (auto mob = std::dynamic_pointer_cast<Mob>(e))
        {
            println("Arrow hit {}", e->get_class_name());
            mob->damage(1, id());
            m_velocity = glm::vec3(0.0f);
            break;
        }
    }
}

void ArrowEntity::draw(const RenderPass& pass)
{
    m_model->encode(pass, get_global_transform());
}
