#pragma once

#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"
#include "UI/ItemSlot.hpp"
#include "UI/Widget.hpp"

constexpr size_t inventory_width = 9;
constexpr size_t inventory_height = 3;

class QuickSlotWidget : public Widget
{
    CLASS(QuickSlot, Widget);

public:
    QuickSlotWidget();

    virtual void update(float d) override;
    virtual void process_event(Event& event) override { (void)event; }
    virtual void draw(const RenderPass& pass) override;

    void set_item(Id<Item> item);
    void set_count(size_t count);

    void set_selected(bool b) { m_selected = b; }

private:
    std::shared_ptr<ColorRectWidget> m_background;
    std::shared_ptr<TextureRectWidget> m_item_rect;
    std::shared_ptr<LabelWidget> m_label;
    Id<Item> m_item;
    size_t m_count;
    bool m_selected = false;
};

class PlayerInventory : public Inventory
{
    CLASS(PlayerInventory, Inventory);

public:
    PlayerInventory(std::shared_ptr<InventoryContainer> container);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override { (void)event; }

    void draw_toolbar(const RenderPass& pass);

    void set_selected_slot(size_t slot);

    size_t selected_slot() const { return m_selected_slot; }

    void add_stack(ItemStack stack);

    virtual bool on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container) override;
    virtual bool on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container) override;

    void update_recipe();
    void consume_ingredients();

private:
    std::array<std::shared_ptr<QuickSlotWidget>, inventory_width> m_quick_slots;
    std::shared_ptr<Widget> m_quick_slots_container;

    size_t m_selected_slot = 0;
    bool m_dirty = false;
};
