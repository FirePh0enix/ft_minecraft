#include "Item/Bucket.hpp"

#include "Engine.hpp"
#include "World/World.hpp"

BucketItem::BucketItem()
{
    set_texture(EXPECT(Engine::get().registry().create_texture("assets/textures/water.png")));
}

void BucketItem::interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal)
{
    (void)stack;

    Dimension& dim = world.get_dimension(dimension);
    BlockState state = dim.get_block(pos.x + int64_t(normal.x), pos.y + int64_t(normal.y), pos.z + int64_t(normal.z));
    if (!state.is_air())
    {
        return;
    }

    dim.set_tag(pos + normal, "water", int64_t(0));
}
