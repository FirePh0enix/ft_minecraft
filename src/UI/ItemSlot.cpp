#include "UI/ItemSlot.hpp"
#include "Core/Format.hpp"
#include "Engine.hpp"

ItemSlot::ItemSlot()
{
    m_background = EXPECT(newref<ColorRect>());
    set_scale(glm::vec2(0.12));

    m_item_rect = EXPECT(newref<TextureRect>());
    m_label = EXPECT(newref<Label>(Engine::get().get_font()));
}

void ItemSlot::update(float d)
{
    (void)d;

    m_background->set_position(m_position);
    m_background->set_scale(m_scale);

    m_item_rect->set_position(m_position);
    m_item_rect->set_scale(m_scale * 0.9f);

    m_label->set_position(m_position);
    m_label->set_scale(m_scale * 0.8f);
    m_label->update(d);

    if (is_mouse_hovering())
    {
        m_background->set_color(Colors::red);
    }
    else if (m_selected)
    {
        m_background->set_color(Colors::yellow);
    }
    else
    {
        m_background->set_color(Colors::blue);
    }
}

void ItemSlot::process_event(Event& event)
{
    if (event.handled)
        return;
}

void ItemSlot::draw(const RenderPassNode& node)
{
    m_background->draw(node);

    if (!m_empty)
    {
        m_item_rect->draw(node);
        m_label->draw(node);
    }
}

void ItemSlot::set_texture(Ref<Texture> texture)
{
    m_item_rect->set_texture(texture);
    m_empty = false;
}

void ItemSlot::set_count(size_t count)
{
    String text = format("{}", count);
    m_label->set_text(text);
}
