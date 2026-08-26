#pragma once

#include "Id.hpp"
#include "Variant.hpp"

#include <cstddef>
#include <optional>
#include <string>

constexpr size_t itemstack_max_size = 64;

class Item;

class ItemStack
{
public:
    ItemStack()
        : m_item(), m_count(0)
    {
    }

    ItemStack(Id<Item> item, size_t count = 1, const std::map<std::string, Variant>& tags = {})
        : m_item(item), m_count(count), m_tags(tags)
    {
    }

    ItemStack(const ItemStack& is)
        : m_item(is.m_item), m_count(is.m_count), m_tags(is.m_tags)
    {
    }

    ItemStack& operator=(const ItemStack& stack)
    {
        m_item = stack.m_item;
        m_count = stack.m_count;
        m_tags = stack.m_tags;
        return *this;
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
    std::optional<ItemStack> merge(const ItemStack& stack);

    void sub(size_t count);

    void set_tag(const std::string& name, Variant variant); // { m_tags[name] = variant; }
    void remove_tag(const std::string& name) { m_tags.erase(m_tags.find(name)); }

    template <typename T>
    std::optional<T> get_tag(const std::string& name) const
    {
        auto opt = m_tags.find(name);
        if (opt == m_tags.end())
            return std::nullopt;
        return opt->second.get_unchecked<T>();
    }

    const std::map<std::string, Variant>& get_tags() const { return m_tags; }

private:
    Id<Item> m_item;
    size_t m_count;
    std::map<std::string, Variant> m_tags;
};
