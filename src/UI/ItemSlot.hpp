#pragma once

#include "Core/Class.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"
#include "UI/ColorRect.hpp"
#include "UI/Label.hpp"
#include "UI/TextureRect.hpp"
#include "UI/UI.hpp"

class Inventory;
class InventoryContainer;

class ItemSlot : public UI
{
    CLASS(ItemSlot, UI);

public:
    ItemSlot(uint32_t layer, uint32_t index, Inventory *inventory, InventoryContainer *container);
    virtual ~ItemSlot() {}

    void set_item(Id<Item> item);
    void set_count(size_t count);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

    uint32_t layer() const { return m_layer; }
    uint32_t index() const { return m_index; }
    Inventory *inventory() const { return m_inventory; }
    InventoryContainer *container() const { return m_container; }

private:
    Ref<ColorRect> m_background;
    Ref<TextureRect> m_item_rect;
    Ref<Label> m_label;
    Id<Item> m_item;
    size_t m_count;
    uint32_t m_layer;
    uint32_t m_index;
    InventoryContainer *m_container;
    Inventory *m_inventory;
};
