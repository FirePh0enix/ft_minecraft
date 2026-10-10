#include "Item/Item.hpp"

#include "Engine.hpp"
#include "World/World.hpp"

static bool has_major_entities(const std::vector<std::shared_ptr<Entity>>& entities)
{
    for (const std::shared_ptr<Entity>& entity : entities)
    {
        if (!entity->is<ItemBlock>())
            return true;
    }
    return false;
}

void ItemBlock::interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory)
{

    (void)inventory;

    if (stack.count() == 0 || !hit)
    {
        return;
    }

    glm::dvec3 pos(result.block_pos);
    glm::dvec3 normal = result.normal;

    if (m_block == Blocks::portal)
    {
        // Activate from the clicked frame or surface, even when the player is in the opening.
        if (world.try_create_portal(dimension, {result.block_pos.x, result.block_pos.y, result.block_pos.z}) ||
            world.try_create_portal(dimension, {int64_t(pos.x) + int64_t(normal.x),
                                              int64_t(pos.y) + int64_t(normal.y),
                                              int64_t(pos.z) + int64_t(normal.z)}))
            stack.set_count(stack.count() - 1);
        return;
    }

    std::vector<std::shared_ptr<Entity>> entities = world.get_dimension(dimension).cast_box(AABBd(-glm::dvec3(0.5), glm::dvec3(0.5)).translate(glm::dvec3(result.block_pos) + result.normal));
    if (has_major_entities(entities))
    {
        return;
    }

    BlockState state = world.get_block_state(dimension, pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z));
    if (!state.is_air())
    {
        return;
    }

    world.get_dimension(dimension).remove_tag(pos + normal, "water");

    world.set_block_state(dimension, pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z),
                          BlockState(Engine::get().registry().to_block(stack.item()).value().hash));
    stack.set_count(stack.count() - 1);
}
