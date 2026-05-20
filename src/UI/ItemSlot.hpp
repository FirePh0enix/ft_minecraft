#pragma once

#include "Core/Class.hpp"
#include "UI/ColorRect.hpp"
#include "UI/Label.hpp"
#include "UI/TextureRect.hpp"
#include "UI/UI.hpp"

class ItemSlot : public UI
{
    CLASS(ItemSlot, UI);

public:
    ItemSlot();
    virtual ~ItemSlot() {}

    void set_empty(bool v)
    {
        m_empty = v;
    }

    void set_selected(bool v) { m_selected = v; }
    void set_texture(Ref<Texture> texture);
    void set_count(size_t count);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPassNode& node) override;

private:
    Ref<ColorRect> m_background;
    Ref<TextureRect> m_item_rect;
    Ref<Label> m_label;
    bool m_empty = true;
    bool m_selected = false;
};
