#pragma once

#include "Id.hpp"
#include "Item/Item.hpp"

#include <cstddef>

constexpr size_t itemstack_max_size = 64;

class ItemStack
{
public:
    ItemStack()
        : m_item(), m_count(0)
    {
    }

    ItemStack(Id<Item> item, size_t count = 1)
        : m_item(item), m_count(count)
    {
    }

    size_t count() const { return m_count; }
    void set_count(size_t count)
    {
        m_count = count;
        if (count == 0)
            m_item = Id<Item>();
    }

    Id<Item> item() const { return m_item; }

    /**
     * @brief Merge `stack` into `this` returning excess items if any.
     *
     * If `stack` can't merge into `this then stack is returned.
     */
    Option<ItemStack> merge(const ItemStack& stack);

private:
    Id<Item> m_item;
    size_t m_count;
};
