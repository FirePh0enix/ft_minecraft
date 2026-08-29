#include "Item/Crystal.hpp"

#include "Engine.hpp"

CrystalItem::CrystalItem()
{
    set_texture(Engine::get().registry().create_texture("assets/textures/water.png"));
}

void CrystalItem::interact(World& world, int dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal, InventoryContainer& inventory)
{
    (void)world;
    (void)dimension;
    (void)stack;
    (void)pos;
    (void)normal;
    (void)inventory;

    const int other_dimension = (dimension + 1) % 2;

    // clang-format off
    const Id<Block> blocks[]{
        Id<Block>(), Blocks::log, Blocks::log, Id<Block>(),
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Id<Block>(), Blocks::log, Blocks::log, Id<Block>(),
    };
    const Id<Block> blocks2[]{
        Blocks::log, Blocks::log, Blocks::log, Blocks::log,
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Blocks::log, Id<Block>(), Id<Block>(), Blocks::log,
        Blocks::log, Blocks::log, Blocks::log, Blocks::log,
    };
    // clang-format on

    const glm::i64vec3 positions[]{
        glm::i64vec3(1, 0, 0),
        glm::i64vec3(2, 0, 0),
        glm::i64vec3(0, 1, 0),
        glm::i64vec3(0, 2, 0),
        glm::i64vec3(0, 3, 0),
        glm::i64vec3(3, 1, 0),
        glm::i64vec3(3, 2, 0),
        glm::i64vec3(3, 3, 0),
        glm::i64vec3(1, 3, 0),
        glm::i64vec3(2, 3, 0),
    };

    const glm::i64vec3 global_pos = pos;

    bool match;
    size_t i;
    for (i = 0; i < sizeof(positions) / sizeof(positions[0]); i++)
    {
        match = true;
        const glm::i64vec3 position = positions[i];

        for (int64_t x = 0; x < 4; x++)
            for (int64_t y = 0; y < 5; y++)
            {
                glm::i64vec3 p = global_pos - position + glm::i64vec3(x, y, 0);
                BlockState block = world.get_block_state(dimension, p.x, p.y, p.z);

                // world.dd().draw_cube(p, glm::vec3(1), Colors::yellow, 2.0f);

                if (blocks[x + y * 4].valid() && block.id != blocks[x + y * 4].hash)
                {
                    match = false;
                    goto endloop;
                }
            }

        if (match)
            break;

    endloop:
        (void)0;
    }

    if (match)
    {
        const glm::i64vec3 position = positions[i];

        for (int64_t x = 0; x < 4; x++)
            for (int64_t y = 0; y < 5; y++)
            {
                glm::i64vec3 p = global_pos - position + glm::i64vec3(x, y, 0);
                if (blocks2[x + y * 4].valid())
                {
                    world.set_block_state(other_dimension, p.x, p.y, p.z, Engine::get().registry().get_default_state(blocks2[x + y * 4]));
                }
            }

        for (int64_t x = 1; x < 3; x++)
            for (int64_t y = 1; y < 4; y++)
            {
                glm::i64vec3 p = global_pos - position + glm::i64vec3(x, y, 0);
                world.set_block_state(dimension, p.x, p.y, p.z, Engine::get().registry().get_default_state(Blocks::portal));
                world.set_block_state(other_dimension, p.x, p.y, p.z, Engine::get().registry().get_default_state(Blocks::portal));
            }
    }
}
