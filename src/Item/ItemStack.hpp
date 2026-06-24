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

    ItemStack(Id<Item> item, size_t count = 1, const Map<String, Variant>& tags = {})
        : m_item(item), m_count(count), m_tags(tags)
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

    void sub(size_t count);

    void set_tag(const StringView& name, Variant variant) { m_tags.put(name, variant); }
    void remove_tag(const StringView& name) { m_tags.erase(name); }

    template <typename T>
    Option<T> get_tag(const StringView& name) const
    {
        auto opt = m_tags.get(name);

        if (!opt.has_value())
            return None;

        return opt.value().get_unchecked<T>();
    }

    Map<String, Variant> get_tags() const { return m_tags; }

private:
    Id<Item> m_item;
    size_t m_count;
    Map<String, Variant> m_tags;
};
