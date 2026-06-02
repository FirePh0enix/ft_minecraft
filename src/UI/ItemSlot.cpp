#include "UI/ItemSlot.hpp"

#include "Core/Format.hpp"
#include "Engine.hpp"
#include "Item/ItemStack.hpp"

ItemSlot::ItemSlot(glm::i64vec2 pos, Inventory *inventory)
    : m_count(0), m_pos(pos), m_inventory(inventory)
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

    if (is_mouse_hovering() && Input::is_action_just_pressed("ui_click"))
    {
        Option<ItemStack> grabbed = m_inventory->get_grabbed();
        if (grabbed.has_value())
        {
            if (m_item.valid())
            {
                m_inventory->ungrab();
                m_inventory->stackxy(m_pos) = grabbed.get();
            }
            else
            {
                if (grabbed.get().item() == m_inventory->stackxy(m_pos).item())
                {
                    size_t excess = m_inventory->stackxy(m_pos).count() + grabbed.get().count() > itemstack_max_size ? (m_inventory->stackxy(m_pos).count() + grabbed.get().count()) % itemstack_max_size : 0;
                    m_inventory->stackxy(m_pos).set_count((m_inventory->stackxy(m_pos).count() + grabbed.get().count()) % itemstack_max_size);

                    if (excess > 0)
                        m_inventory->grab(ItemStack(grabbed.get().item(), excess), glm::i64vec2(-1));
                    else
                        m_inventory->ungrab();
                }
            }
        }
        else if (!m_item.valid())
        {
            m_inventory->grab(ItemStack(m_item, m_count), m_pos);
            m_inventory->stackxy(m_pos) = ItemStack(Id<Item>(), 0);
        }
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

    if (m_item.valid())
    {
        m_item_rect->draw(node);
        m_label->draw(node);
    }
}

void ItemSlot::set_item(Id<Item> item)
{
    m_item = item;

    if (item.valid())
        m_item_rect->set_texture(Engine::get().registry().get_texture(item));
}

void ItemSlot::set_count(size_t count)
{
    String text = format("{}", count);
    m_label->set_text(text);
    m_count = count;
}
