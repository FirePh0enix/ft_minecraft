#pragma once

#include "Item/Item.hpp"

class BowItem : public Item
{
    CLASS(Bow, Item);

public:
    BowItem();
    virtual void interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal) override;
    virtual void on_release(World& world, size_t dimension, ItemStack& stack) override;
};