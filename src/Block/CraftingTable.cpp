#include "Block/CraftingTable.hpp"
#include "Inventory/CraftingTable.hpp"
#include "Inventory/Inventory.hpp"

CraftingTableBlock::CraftingTableBlock()
{
    set_texture("assets/textures/log.png");
}

void CraftingTableBlock::open_inventory(glm::ivec3 pos, Player *player)
{
    (void)pos;
    (void)player;

    std::shared_ptr<InventoryContainer> container = std::make_shared<InventoryContainer>();
    container->add_layer(9);
    container->add_layer(1);

    std::shared_ptr<CraftingTableInventory> inv = std::make_shared<CraftingTableInventory>(container, player->get_inventory_container());
    player->open_inventory(inv);
}

void CraftingTableBlock::close_inventory(glm::ivec3 pos, Player *player)
{
    (void)pos;
    (void)player;
}
