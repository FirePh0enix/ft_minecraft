#include "Item/Crystal.hpp"

#include "World/World.hpp"

CrystalItem::CrystalItem()
{
}

void CrystalItem::interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory)
{
    (void)stack;
    (void)inventory;
    if (hit)
        world.try_create_portal(dimension, {result.block_pos.x, result.block_pos.y, result.block_pos.z});
}
