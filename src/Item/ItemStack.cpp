#include "Item/ItemStack.hpp"

Option<ItemStack> ItemStack::merge(const ItemStack& stack)
{
    if (m_item != stack.m_item || m_count >= itemstack_max_size)
        return stack;

    size_t excess = count() + stack.count() > itemstack_max_size ? (count() + stack.count()) % (itemstack_max_size + 1) : 0;
    m_count = (count() + stack.count()) % (itemstack_max_size + 1);

    if (excess > 0)
    {
        ItemStack new_stack(m_item, excess);
        return new_stack;
    }
    else
    {
        return None;
    }
}

void ItemStack::sub(size_t count)
{
    if ((ssize_t)m_count - (ssize_t)count < 0)
    {
        m_item = Id<Item>();
        m_count = 0;
    }
    else
    {
        m_count -= count;
    }
}
