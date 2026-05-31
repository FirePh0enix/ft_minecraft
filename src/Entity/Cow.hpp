#pragma once

#include "Entity/Entity.hpp"
#include "Mob.hpp"
#include <cstddef>

class Cow : public Mob
{
public:
    Cow() : Mob(3, 0, 3.0f, 1.0f)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    void start() override;
    void tick(float delta) override;
    void draw(const RenderPassNode& node) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;

protected:
    void die() override;
    void flee_from(int radius);

    Entity *m_threat_entity = nullptr;
};
