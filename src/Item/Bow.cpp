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

    auto start = stack.get_tag<int64_t>("draw_start");

    if (start.has_value())
    {
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        stack.set_tag("draw_start", now_ms);
    }
}

void BowItem::on_release(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::vec3 dir)
{
    (void)stack;

    float power = 0.1f;
    auto start = stack.get_tag<int64_t>("draw_start");

    if (start.has_value())
    {
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        float held_seconds = (float)(now_ms - start.value()) / 1000.0f;
        power = std::clamp(held_seconds, 0.1f, 1.0f);

        stack.remove_tag("draw_start");
    }

    Ref<ArrowEntity> arrow = newref<ArrowEntity>(Items::stone_block);
    auto arrow_pos = pos;
    arrow_pos.z += 1.0f;
    arrow->get_transform().position() = arrow_pos;
    arrow->m_dir = dir * power;
    world.add_entity(dimension, arrow);

    println("power: {}", power);
}