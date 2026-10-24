#include "Entity/Arrow.hpp"

#include "Engine.hpp"
#include "Entity/LivingEntity.hpp"
#include "Entity/Player.hpp"
#include "Entity/ProjectileCollision.hpp"
#include "Inventory/Inventory.hpp"
#include "Network/Network.hpp"
#include "Profiler.hpp"
#include "World/Registry.hpp"
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
    ZoneScoped;
    if (Engine::get().is_client() || !m_active)
        return;

    const auto& dimension = m_world->get_dimension(m_dimension);
    if (m_embedded)
    {
        // Expire on the server so all clients receive the same removal.
        m_embedded_seconds += std::max(0.0, double(delta));
        if (m_embedded_seconds >= embedded_lifetime_seconds)
        {
            remove();
            return;
        }
        const AABBd pickup_box = m_aabb.translate(get_position()).grow(glm::dvec3(0.25));
        for (const auto& player : dimension.get_players())
        {
            if (player->get_dimension() != m_dimension || !player->is_active() || player->is_dead())
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
        // Include the segment endpoint so crossing a block boundary cannot
        // miss the block ahead. Keep storage for subsequent segments/ticks.
        dimension.get_boxes_that_may_collide(m_aabb.translate(get_position()).expand(step), m_collision_boxes);
        for (const AABBd& box : m_collision_boxes)
        {
            if (auto t = projectile_contact(ray, box.grow(glm::dvec3(0.04)), nearest))
            {
                nearest = *t;
                hit = true;
            }
        }
        for (const auto& living : dimension.get_living_entities())
        {
            if (living->id() == m_owner || living->get_dimension() != m_dimension ||
                !living->is_active() || living->is_dead())
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
