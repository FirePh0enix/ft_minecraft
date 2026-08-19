#pragma once

#include "Item/Item.hpp"

class CrystalItem : public Item
{
    CLASS(CrystalItem, Item);

public:
    CrystalItem();

    virtual void interact(World& world, int dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory) override;

private:
};
