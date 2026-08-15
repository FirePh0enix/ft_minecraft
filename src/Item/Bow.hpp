#pragma once

#include "Core/Math.hpp"
#include "Item/Item.hpp"

class BowItem : public Item
{
    CLASS(BowItem, Item);

public:
    BowItem();
    virtual void interact(World& world, int dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory) override;
    virtual void on_release(World& world, int dimension, ItemStack& stack, glm::i64vec3 pos, glm::vec3 dir, InventoryContainer& inventory) override;
};