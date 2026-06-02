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

    Ref<InventoryContainer> container = EXPECT(newref<InventoryContainer>());
    EXPECT(container->add_layer(9));
    EXPECT(container->add_layer(1));

    Ref<CraftingTableInventory> inv = EXPECT(newref<CraftingTableInventory>(container, player->get_inventory_container()));
    player->open_inventory(inv);
}

void CraftingTableBlock::close_inventory(glm::ivec3 pos, Player *player)
{
    (void)pos;
    (void)player;
}
