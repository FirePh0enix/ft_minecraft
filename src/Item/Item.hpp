#pragma once

#include "Core/Class.hpp"
#include "Id.hpp"
#include "World/Block.hpp"

class Item : public Object
{
    CLASS(Item, Object);

public:
private:
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
