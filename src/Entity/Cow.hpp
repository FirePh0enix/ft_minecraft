#pragma once

#include "Entity/Entity.hpp"
#include "Entity/Pathfinding/Pathfinding.hpp"
#include "Mob.hpp"

class Cow : public Mob
{
public:
    Cow() : Mob(3, 0, 1.0f)
    {
    }

    void start() override;
    void tick(float delta) override;
    void draw(RenderPassEncoder& encoder) override;
    void on_ready() override;
    void on_hit_by(Entity& entity) override;

protected:
    void die() override;
    void flee_from(const Entity& threat, int radius);

    Pathfinding *m_pathfinding = nullptr;
};
