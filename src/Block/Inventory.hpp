#pragma once

#include "Block/Block.hpp"
#include "Entity/Player.hpp"

class InventoryBlock : public Block
{
    CLASS(Inventory, Block);

public:
    virtual void open_inventory(glm::ivec3 pos, Player *player) = 0;
    virtual void close_inventory(glm::ivec3 pos, Player *player) = 0;

private:
};
