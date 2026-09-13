#pragma once

#include "Item/Item.hpp"

class CrystalItem : public Item
{
    CLASS(CrystalItem, Item);

public:
    CrystalItem();

    virtual void interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory) override;

private:
};
