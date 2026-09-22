#include "Item/Bow.hpp"

#include "Engine.hpp"
#include "Entity/Arrow.hpp"
#include "Entity/Player.hpp"
#include "Inventory/Inventory.hpp"
#include "Network/Network.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

BowItem::BowItem()
{
    set_texture(Engine::get().registry().create_texture("data/resourcepacks/pixel-perfection/assets/minecraft/textures/item/bow.png"));
}

void BowItem::interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory)
{
    (void)world;
    (void)dimension;
    (void)inventory;
    (void)hit;
    (void)result;

    const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    stack.set_tag("draw_start", now);
}

void BowItem::on_release(World& world, int dimension, ItemStack& stack, glm::dvec3 pos, glm::vec3 dir, InventoryContainer& inventory, Entity* user)
{
    const auto start = stack.get_tag<int64_t>("draw_start");
    if (!start.has_value())
        return;
    stack.remove_tag("draw_start");

    const float length_squared = glm::length2(dir);
    if (!std::isfinite(length_squared) || length_squared <= 0.0f)
        return;

    const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    const float power = std::clamp(float(now - *start) / 1000.0f, 0.1f, 1.0f);
    if (!inventory.consume(Items::arrow).has_value())
        return;

    // Clients predict ammunition use; only the server creates and simulates arrows.
    if (Engine::get().is_client())
        return;

    const glm::vec3 direction = dir / std::sqrt(length_squared);
    auto arrow = std::make_shared<ArrowEntity>();
    arrow->set_position(pos);
    arrow->set_owner(user ? user->id() : EntityId());
    arrow->set_velocity(direction * (30.0f * power));
    arrow->orient(direction);
    world.add_entity(dimension, arrow);

    const AddEntityPacket packet(arrow->get_position(), arrow->get_rotation(), arrow->id(), arrow->get_class_hash_code());
    Engine::get().server()->route_packet(NetworkConnection::create_packet(packet));

    if (auto* player = dynamic_cast<Player*>(user))
        player->call_rpc("play_one_shot_sound", static_cast<int64_t>(EntitySound::BowRelease));
}
