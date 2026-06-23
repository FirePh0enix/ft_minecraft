#include "Item/Bow.hpp"

#include "Core/Print.hpp"
#include "Engine.hpp"
#include "Entity/Arrow.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

BowItem::BowItem()
{
    set_texture(Engine::get().registry().create_texture("assets/textures/snow.png"));
}

void BowItem::interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal)
{
    (void)world;
    (void)dimension;
    (void)stack;
    (void)pos;
    (void)normal;
    println("Bow holding !");
}

void BowItem::on_release(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::vec3 dir)
{
    (void)stack;

    Ref<ArrowEntity> arrow = newref<ArrowEntity>(Items::stone_block);
    auto arrow_pos = pos;
    arrow_pos.z += 1.0f;
    arrow->get_transform().position() = arrow_pos;
    arrow->m_dir = dir;
    world.add_entity(dimension, arrow);

    println("Bow released !");
}