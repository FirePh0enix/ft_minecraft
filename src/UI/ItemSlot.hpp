#pragma once

#include "Core/Class.hpp"
#include "UI/ColorRect.hpp"
#include "UI/Label.hpp"
#include "UI/TextureRect.hpp"
#include "UI/UI.hpp"
#include "World/Block.hpp"

class Inventory;

class ItemSlot : public UI
{
    CLASS(ItemSlot, UI);

public:
    ItemSlot(glm::i64vec2 pos, Inventory *inventory);
    virtual ~ItemSlot() {}

    void set_block(uint16_t block_id);
    void set_count(size_t count);
    void set_selected(bool b) { m_selected = b; }

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

private:
    Ref<ColorRect> m_background;
    Ref<TextureRect> m_item_rect;
    Ref<Label> m_label;
    Ref<Block> m_block;
    size_t m_count;
    glm::i64vec2 m_pos;
    Inventory *m_inventory;
    bool m_selected = false;
};
