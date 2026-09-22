#include "Item/Arrow.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

ArrowItem::ArrowItem()
{
    set_texture(Engine::get().registry().create_texture("data/resourcepacks/pixel-perfection/assets/minecraft/textures/item/arrow.png"));
}
