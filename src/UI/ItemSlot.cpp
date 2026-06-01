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
            if (m_block.is_null())
            {
                m_inventory->ungrab();
                m_inventory->stackxy(m_pos) = grabbed.get();
            }
            else
            {
                if (grabbed.get().get_block() == m_inventory->stackxy(m_pos).get_block())
                {
                    size_t excess = m_inventory->stackxy(m_pos).count() + grabbed.get().count() > itemstack_max_size ? (m_inventory->stackxy(m_pos).count() + grabbed.get().count()) % itemstack_max_size : 0;
                    m_inventory->stackxy(m_pos).set_count((m_inventory->stackxy(m_pos).count() + grabbed.get().count()) % itemstack_max_size);

                    if (excess > 0)
                        m_inventory->grab(ItemStack(grabbed.get().get_block(), excess), glm::i64vec2(-1));
                    else
                        m_inventory->ungrab();
                }
            }
        }
        else if (!m_block.is_null())
        {
            m_inventory->grab(ItemStack(m_block->id(), m_count), m_pos);
            m_inventory->stackxy(m_pos) = ItemStack(0, 0);
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

    if (!m_block.is_null())
    {
        m_item_rect->draw(node);
        m_label->draw(node);
    }
}

void ItemSlot::set_block(uint16_t block_id)
{
    if (block_id == 0)
    {
        m_block = nullptr;
        return;
    }

    m_block = Engine::get().blocks().get_block_by_id(block_id);
    m_item_rect->set_texture(Engine::get().blocks().get_texture(m_block->get_texture_ids()[0]));
}

void ItemSlot::set_count(size_t count)
{
    String text = format("{}", count);
    m_label->set_text(text);
    m_count = count;
}
