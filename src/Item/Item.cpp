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

void ItemBlock::interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal)
{
    if (stack.count() == 0)
    {
        return;
    }

    std::vector<std::shared_ptr<Entity>> entities = world.get_dimension(dimension).cast_box(AABBf(-glm::vec3(0.5), glm::vec3(0.5)).translate(pos + normal));
    if (has_major_entities(entities))
    {
        return;
    }

    BlockState state = world.get_block_state(pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z));
    if (!state.is_air())
    {
        return;
    }

    world.get_dimension(dimension).remove_tag(pos + normal, "water");

    world.set_block_state(pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z),
                          BlockState(Engine::get().registry().to_block(stack.item()).value()));
    stack.set_count(stack.count() - 1);
}
