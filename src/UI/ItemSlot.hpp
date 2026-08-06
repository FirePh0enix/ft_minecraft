#pragma once

#include "Core/Class.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"
#include "UI/Widget.hpp"

class Inventory;
class InventoryContainer;

class ItemSlotWidget : public Widget
{
    CLASS(ItemSlot, Widget);

public:
    ItemSlotWidget(uint32_t layer, uint32_t index, Inventory *inventory, InventoryContainer *container);

    void set_item(Id<Item> item);
    void set_count(size_t count);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPass& pass) override;

    uint32_t layer() const { return m_layer; }
    uint32_t index() const { return m_index; }
    Inventory *inventory() const { return m_inventory; }
    InventoryContainer *container() const { return m_container; }

private:
    std::shared_ptr<ColorRectWidget> m_background;
    std::shared_ptr<TextureRectWidget> m_item_rect;
    std::shared_ptr<LabelWidget> m_label;
    Id<Item> m_item;
    size_t m_count;
    uint32_t m_layer;
    uint32_t m_index;
    InventoryContainer *m_container;
    Inventory *m_inventory;
};
