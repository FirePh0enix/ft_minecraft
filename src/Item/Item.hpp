#pragma once

#include "Block/Block.hpp"
#include "Core/Class.hpp"
#include "Id.hpp"
#include "Render/Renderer.hpp"

class InventoryContainer;
class Entity;

class Item : public Object
{
    CLASS(Item, Object);

public:
    /**
     * Callback used when a player is interacting with the world with an item in its hand.
     */
    virtual void interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory)
    {
        (void)world;
        (void)dimension;
        (void)stack;
        (void)hit;
        (void)result;
        (void)inventory;
    }

    virtual void on_release(World& world, int dimension, ItemStack& stack, glm::dvec3 pos, glm::vec3 dir, InventoryContainer& inventory, Entity* user)
    {
        (void)world;
        (void)dimension;
        (void)stack;
        (void)user;
        (void)pos;
        (void)dir;
        (void)inventory;
    }

    std::shared_ptr<Texture> get_texture() const { return m_texture; }
    void set_texture(const std::shared_ptr<Texture>& texture) { m_texture = texture; }

private:
    std::shared_ptr<Texture> m_texture;
};

class ItemBlock : public Item
{
    CLASS(ItemBlock, Item);

public:
    ItemBlock(Id<Block> block)
        : m_block(block)
    {
    }

    virtual void interact(World& world, int dimension, ItemStack& stack, bool hit, const RaycastResult& result, InventoryContainer& inventory) override;

    Id<Block> block() const { return m_block; }

private:
    Id<Block> m_block;
};
