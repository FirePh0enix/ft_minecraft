#pragma once

#include "Core/Class.hpp"
#include "Id.hpp"
#include "Item/Item.hpp"
#include "UI/ColorRect.hpp"
#include "UI/Label.hpp"
#include "UI/TextureRect.hpp"
#include "UI/UI.hpp"

class Inventory;

class ItemSlot : public UI
{
    CLASS(ItemSlot, UI);

public:
    ItemSlot(glm::i64vec2 pos, Inventory *inventory);
    virtual ~ItemSlot() {}

    void set_item(Id<Item> item);
    void set_count(size_t count);
    void set_selected(bool b) { m_selected = b; }

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

private:
    Ref<ColorRect> m_background;
    Ref<TextureRect> m_item_rect;
    Ref<Label> m_label;
    Id<Item> m_item;
    size_t m_count;
    glm::i64vec2 m_pos;
    Inventory *m_inventory;
    bool m_selected = false;
};
