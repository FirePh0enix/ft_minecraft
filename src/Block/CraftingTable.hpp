#pragma once

#include "Block/Inventory.hpp"

class CraftingTableBlock : public InventoryBlock
{
    CLASS(CraftingTable, InventoryBlock);

public:
    CraftingTableBlock();

    virtual void open_inventory(glm::ivec3 pos, Player *player) override;
    virtual void close_inventory(glm::ivec3 pos, Player *player) override;
};
