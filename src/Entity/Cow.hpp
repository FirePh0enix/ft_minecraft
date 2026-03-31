#pragma once

#include "AABB.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Mob.hpp"

class Cow : public Mob
{
public:
    Cow() : Mob(3, 0)
    {
        m_aabb = AABB(
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f));
    }

    void start() override;
    void tick(float delta) override;
    void draw(RenderPassEncoder& encoder) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;

protected:
    void die() override;

    Pathfinding *m_pathfinding = nullptr;
};