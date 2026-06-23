#pragma once

#include "Block/Block.hpp"
#include "Core/Class.hpp"
#include "Id.hpp"
#include "Render/Renderer.hpp"

class Item : public Object
{
    CLASS(Item, Object);

public:
    /**
     * Callback used when a player is interacting with the world with an item in its hand.
     */
    virtual void interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal)
    {
        (void)world;
        (void)dimension;
        (void)stack;
        (void)pos;
        (void)normal;
    }

    virtual void on_release(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::vec3 dir)
    {
        (void)world;
        (void)dimension;
        (void)stack;
        (void)pos;
        (void)dir;
    }

    Ref<Texture> get_texture() const { return m_texture; }
    void set_texture(const Ref<Texture>& texture) { m_texture = texture; }

private:
    Ref<Texture> m_texture;
};

class ItemBlock : public Item
{
    CLASS(ItemBlock, Item);

public:
    ItemBlock(Id<Block> block)
        : m_block(block)
    {
    }

    virtual void interact(World& world, size_t dimension, ItemStack& stack, glm::i64vec3 pos, glm::i64vec3 normal) override;

    Id<Block> block() const { return m_block; }

private:
    Id<Block> m_block;
};
