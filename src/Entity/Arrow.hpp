#pragma once

#include "Entity/Entity.hpp"
#include "Model.hpp"

class ArrowEntity : public Entity
{
    CLASS(ArrowEntity, Entity);

public:
    ArrowEntity();
    void tick(float delta) override;
    void draw(const RenderPass& pass, bool shadowmap) override;
    void on_ready() override;
    void set_velocity(glm::vec3 velocity) { m_velocity = velocity; }
    void set_owner(EntityId owner) { m_owner = owner; }
    void orient(glm::vec3 direction);

private:
    EntityId m_owner;
    bool m_embedded = false;
    void remove();
    std::shared_ptr<ModelLegacy> m_model;
};
