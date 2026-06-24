#pragma once

#include "Item/Item.hpp"

class BowItem : public Item
{
    CLASS(Bow, Item);

public:
    BowItem();
    virtual void interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory) override;
    virtual void on_release(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::vec3 dir, InventoryContainer& inventory) override;
};