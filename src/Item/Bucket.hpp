#pragma once

#include "Item/Item.hpp"

class BucketItem : public Item
{
    CLASS(BucketItem, Item);

public:
    BucketItem();

    virtual void interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory) override;

private:
};
