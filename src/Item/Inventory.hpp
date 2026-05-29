#pragma once

#include "Item/ItemStack.hpp"
#include "UI/Container.hpp"
#include "UI/ItemSlot.hpp"

constexpr size_t inventory_width = 9;
constexpr size_t inventory_height = 3;

class Inventory : public Container
{
    CLASS(Inventory, Container);

public:
    Inventory();

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

    void set_open(bool v) { m_open = v; }
    void set_selected_slot(size_t slot);

    ItemStack& get_quick_stack(size_t index) { return m_quick_data[index]; }
    void set_quick_stack(size_t index, ItemStack stack) { m_quick_data[index] = stack; }

    void add_stack(ItemStack stack);

private:
    Array<ItemStack, inventory_width * inventory_height> m_data;
    Array<ItemStack, inventory_width> m_quick_data;

    Array<Ref<ItemSlot>, inventory_width * inventory_height> m_slots;
    Array<Ref<ItemSlot>, inventory_width> m_quick_slots;

    Ref<Container> m_slots_container;
    Ref<Container> m_quick_slots_container;

    size_t m_selected_slot = 0;
    bool m_open = false;
};
