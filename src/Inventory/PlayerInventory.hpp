#pragma once

#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"
#include "UI/Container.hpp"
#include "UI/ItemSlot.hpp"
#include "UI/UI.hpp"

constexpr size_t inventory_width = 9;
constexpr size_t inventory_height = 3;

class QuickSlot : public UI
{
    CLASS(QuickSlot, UI);

public:
    QuickSlot();

    virtual void update(float d) override;
    virtual void process_event(Event& event) override { (void)event; }
    virtual void draw(const RenderPass& pass) override;

    void set_item(Id<Item> item);
    void set_count(size_t count);

    void set_selected(bool b) { m_selected = b; }

private:
    Ref<ColorRect> m_background;
    Ref<TextureRect> m_item_rect;
    Ref<Label> m_label;
    Id<Item> m_item;
    size_t m_count;
    bool m_selected = false;
};

class PlayerInventory : public Inventory
{
    CLASS(PlayerInventory, Inventory);

public:
    PlayerInventory(Ref<InventoryContainer> container);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override { (void)event; }
    virtual void draw(const RenderPass& pass) override;

    void draw_toolbar(const RenderPass& pass);

    void set_selected_slot(size_t slot);

    size_t selected_slot() const { return m_selected_slot; }

    void add_stack(ItemStack stack);

private:
    Array<Ref<QuickSlot>, inventory_width> m_quick_slots;
    Ref<Container> m_quick_slots_container;

    size_t m_selected_slot = 0;
};
