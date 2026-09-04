#pragma once

#include "Entity/Entity.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"

class ItemEntity : public Entity
{
    CLASS(ItemEntity, Entity);

public:
    ItemEntity(Id<Item> item);

    virtual void tick(float delta) override;
    virtual void draw(const RenderPass& pass) override;

    Id<Item> item() const { return m_item; }

private:
    Id<Item> m_item;
    std::shared_ptr<BindGroup> m_bg;
    std::shared_ptr<Buffer> m_model_buffer;
    std::shared_ptr<Mesh> m_mesh;

    float m_time;
};
