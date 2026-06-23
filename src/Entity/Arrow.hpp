#pragma once

#include "Entity/Entity.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"

class ArrowEntity : public Entity
{
    CLASS(ArrowEntity, Entity);

public:
    ArrowEntity(Id<Item> item);

    virtual void tick(float delta) override;
    virtual void draw(const RenderPass& pass) override;

    Id<Item> item() const { return m_item; }

    glm::vec3 m_dir;

private:
    Id<Item> m_item;
    glm::uvec3 m_textures{};
    Ref<Material> m_material;
    Ref<Buffer> m_model_buffer;
};
