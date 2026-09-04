#include "Item/Bucket.hpp"

#include "Engine.hpp"
#include "Inventory/Inventory.hpp"
#include "World/World.hpp"

BucketItem::BucketItem()
{
    //    set_texture(Engine::get().registry().create_texture("data/resourcepacks/core/assets/minecraft/textures/item/water_bucket.png"));
}

void BucketItem::interact(World& world, int dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory)
{
    (void)stack;
    (void)inventory;

    Dimension& dim = world.get_dimension(dimension);
    BlockState state = dim.get_block(pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z));
    if (!state.is_air())
    {
        return;
    }

    dim.set_tag(pos + normal, "water", int64_t(0));
}
