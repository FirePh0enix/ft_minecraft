#pragma once

#include "Item/Item.hpp"

class BucketItem : public Item
{
    CLASS(BucketItem, Item);

public:
    BucketItem();

    virtual void interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory) override;

private:
};
