#include "Entity/Arrow.hpp"

#include "Engine.hpp"
#include "Entity/LivingEntity.hpp"
#include "Entity/Player.hpp"
#include "Inventory/Inventory.hpp"
#include "World/Registry.hpp"
#include "Entity/ProjectileCollision.hpp"
#include "Network/Network.hpp"
#include "World/World.hpp"

#include <algorithm>
#include <cmath>

ArrowEntity::ArrowEntity()
{
    m_aabb = AABBd(glm::dvec3(-0.04), glm::dvec3(0.04));
}

void ArrowEntity::on_ready()
{
    // Entity models use this renderer too (cow, zombie, and player).
    m_model = EXPECT(ModelLegacy::load("data/models/arrow.json"));
}

void ArrowEntity::orient(glm::vec3 direction)
{
    if (glm::length2(direction) < 1e-8f)
        return;
    direction = glm::normalize(direction);
    const glm::vec3 up = std::abs(direction.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    set_rotation(glm::quatLookAt(direction, up));
}

void ArrowEntity::tick(float delta)
{
    if (Engine::get().is_client())
        return;

    if (m_embedded)
    {
        const AABBd pickup_box = m_aabb.translate(get_position()).grow(glm::dvec3(0.25));
        for (const auto& entity : m_world->get_dimension(m_dimension).get_entities())
        {
            auto player = std::dynamic_pointer_cast<Player>(entity);
            if (!player || !player->is_active() || player->is_dead())
                continue;
            if (pickup_box.intersect(player->get_aabb().translate(player->get_position())) &&
                player->get_inventory_container()->add_item(Items::arrow))
            {
                player->sync_inventory();
                remove();
                return;
            }
        }
        return;
    }
    if (!chunk_is_loaded() || delta <= 0.0f)
        return;

    m_velocity.y -= 9.81 * delta;
    orient(glm::vec3(m_velocity));
    const glm::dvec3 displacement = m_velocity * double(delta);
    const double distance = glm::length(displacement);
    if (distance <= 0.0)
        return;

    // Short swept segments catch fast arrows and keep the block query local.
    const int steps = std::max(1, int(std::ceil(distance / 0.5)));
    const glm::dvec3 step = displacement / double(steps);
    const double step_length = glm::length(step);
    for (int i = 0; i < steps; ++i)
    {
        if (!chunk_is_loaded())
            return;
        const Ray ray(get_position(), step / step_length);
        double nearest = step_length;
        bool hit = false;
        std::shared_ptr<LivingEntity> victim;
        const auto& dimension = m_world->get_dimension(m_dimension);
        for (const AABBd& box : dimension.get_boxes_that_may_collide(m_aabb.translate(get_position())))
        {
            if (auto t = projectile_contact(ray, box.grow(glm::dvec3(0.04)), nearest))
            {
                nearest = *t;
                hit = true;
            }
        }
        for (const auto& entity : dimension.get_entities())
        {
            if (entity->id() == m_owner || !entity->is_active())
                continue;
            auto living = std::dynamic_pointer_cast<LivingEntity>(entity);
            if (!living || living->is_dead())
                continue;
            const AABBd box = living->get_aabb().translate(living->get_global_transform().position()).grow(glm::dvec3(0.04));
            if (auto t = projectile_contact(ray, box, nearest); t && (!hit || *t < nearest))
            {
                nearest = *t;
                hit = true;
                victim = living;
            }
        }
        set_position(ray.at(nearest));
        if (hit)
        {
            m_embedded = true;
            m_velocity = glm::dvec3(0);
            if (victim)
            {
                victim->damage(1, m_owner);
                remove();
            }
            return;
        }
    }
}

void ArrowEntity::remove()
{
    m_active = false;
    Engine::get().server()->route_packet(NetworkConnection::create_packet(RemoveEntityPacket(id())));
}

void ArrowEntity::draw(const RenderPass& pass, bool shadowmap)
{
    if (m_model)
        m_model->encode(pass, get_global_transform(), shadowmap);
}
