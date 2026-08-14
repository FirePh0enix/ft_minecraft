#pragma once

#include "Entity/Entity.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"
#include "Model.hpp"
#include <memory>

class ArrowEntity : public Entity
{
    CLASS(ArrowEntity, Entity);

public:
    ArrowEntity(Id<Item> item);

    virtual void tick(float delta) override;
    virtual void draw(const RenderPass& pass) override;
    void on_ready() override;
    inline void set_velocity(const glm::vec3 velocity) { m_velocity = velocity; }

    Id<Item> item() const { return m_item; }

private:
    Id<Item> m_item;
    glm::uvec3 m_textures{};
    std::shared_ptr<Material> m_material;
    std::shared_ptr<Buffer> m_model_buffer;
    std::shared_ptr<Model> m_model;
};
