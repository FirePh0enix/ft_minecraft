#pragma once

#include "Block/Block.hpp"
#include "Core/Class.hpp"
#include "Id.hpp"
#include "Render/Renderer.hpp"

class Item : public Object
{
    CLASS(Item, Object);

public:
    Ref<Texture> get_texture() const { return m_texture; }
    void set_texture(const Ref<Texture>& texture) { m_texture = texture; }

private:
    Ref<Texture> m_texture;
};

class ItemBlock : public Item
{
    CLASS(Item, Object);

public:
    ItemBlock(Id<Block> block)
        : m_block(block)
    {
    }

    Id<Block> block() const { return m_block; }

private:
    Id<Block> m_block;
};
