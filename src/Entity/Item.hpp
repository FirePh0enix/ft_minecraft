#pragma once

#include "Entity/Entity.hpp"
#include "World/Block.hpp"

class ItemEntity : public Entity
{
    CLASS(ItemEntity, Entity);

public:
    ItemEntity(Ref<Block> block);

    virtual void tick(float delta) override;
    virtual void draw(const RenderPassNode& node) override;

private:
    Ref<Material> m_material;
    Ref<Block> m_block;
    Ref<Buffer> m_model_buffer;

    float m_time;
};
