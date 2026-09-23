#pragma once

#include "Core/Math.hpp"
#include "Item/Item.hpp"

class BowItem : public Item
{
    CLASS(BowItem, Item);

public:
    BowItem();
    virtual void interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory) override;
    virtual void on_release(World& world, int dimension, ItemStack& stack, glm::dvec3 pos, glm::vec3 dir, InventoryContainer& inventory, Entity *user) override;

    virtual std::shared_ptr<Texture> get_texture(const ItemStack& stack) const override;

private:
    std::shared_ptr<Texture> m_textures[4];
};