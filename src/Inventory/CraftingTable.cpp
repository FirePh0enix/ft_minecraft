#include "Inventory/CraftingTable.hpp"

#include "Inventory/Inventory.hpp"

CraftingTableInventory::CraftingTableInventory(Ref<InventoryContainer> inventory, Ref<InventoryContainer> player_inventory)
    : Inventory(inventory), m_player_inventory(player_inventory)
{
    add_background();
    add_grid(9, 3, 0, glm::vec2(0, -0.2f), m_player_inventory.ptr());
    add_grid(9, 1, 1, glm::vec2(0, -0.6f), m_player_inventory.ptr());

    add_grid(3, 3, 0, glm::vec2(-0.2f, 0.3f));
    add_grid(1, 1, 1, glm::vec2(0.3f, 0.3f));
}

void CraftingTableInventory::update(float d)
{
    Inventory::update(d);
}

void CraftingTableInventory::draw(const RenderPassNode& node)
{
    Inventory::draw(node);
}

bool CraftingTableInventory::on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)index;
    (void)stack;

    if (layer == 1 && container == m_container.ptr())
        return false;
    return true;
}

bool CraftingTableInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)layer;
    (void)index;
    (void)stack;
    (void)container;
    return true;
}
