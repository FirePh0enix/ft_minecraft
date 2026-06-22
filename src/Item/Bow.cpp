#include "Item/Bow.hpp"

#include "Core/Print.hpp"
#include "Engine.hpp"
#include "World/World.hpp"

BowItem::BowItem()
{
    set_texture(Engine::get().registry().create_texture("assets/textures/snow.png"));
}

void BowItem::interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal)
{
    (void)stack;
    println("Bow holding !");
}

void BowItem::on_release(World& world, size_t dimension, ItemStack& stack)
{
    (void)world;
    (void)dimension;
    (void)stack;

    println("Bow released !");
}