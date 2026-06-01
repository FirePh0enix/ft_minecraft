#pragma once

#include "Item/ItemStack.hpp"
#include "UI/Container.hpp"
#include "UI/ItemSlot.hpp"

constexpr size_t inventory_width = 9;
constexpr size_t inventory_height = 3;
constexpr size_t inventory_slot_count = inventory_width * (inventory_height + 1);

class Inventory : public Container
{
    CLASS(Inventory, Container);

public:
    Inventory();

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

    void set_open(bool v);
    void set_selected_slot(size_t slot);

    size_t selected_slot() const { return m_selected_slot; }

    void grab(const ItemStack& itemstack, glm::i64vec2 pos);
    void ungrab();
    Option<ItemStack> get_grabbed();

    View<ItemStack> data() const { return m_data; }

    ItemStack& stack(size_t index) { return m_data[index]; }
    const ItemStack& stack(size_t index) const { return m_data[index]; }

    ItemStack& stackxy(glm::i64vec2 pos) { return m_data[pos.x + pos.y * inventory_width]; }
    const ItemStack& stackxy(glm::i64vec2 pos) const { return m_data[pos.x + pos.y * inventory_width]; }

    ItemStack& get_quick_stack(size_t index) { return m_data[index + inventory_height * inventory_width]; }
    void set_quick_stack(size_t index, ItemStack stack) { m_data[index + inventory_height * inventory_width] = stack; }

    void add_stack(ItemStack stack);

private:
    Array<ItemStack, inventory_slot_count> m_data;
    Array<Ref<ItemSlot>, inventory_slot_count> m_slots;

    Ref<Container> m_slots_container;
    Ref<Container> m_quick_slots_container;

    Ref<TextureRect> m_grabbed_item_rect;
    Ref<Label> m_grabbed_item_label;

    size_t m_selected_slot = 0;
    bool m_open = false;
    Option<ItemStack> m_grabbed_stack;
    glm::i64vec2 m_grabbed_from = glm::i64vec2();

    ItemStack& quick_stack(size_t index) { return m_data[index + inventory_height * inventory_width]; }
    const ItemStack& quick_stack(size_t index) const { return m_data[index + inventory_height * inventory_width]; }
};
