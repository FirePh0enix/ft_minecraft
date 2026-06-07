#include "UI/ItemSlot.hpp"

#include "Core/Format.hpp"
#include "Engine.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"

ItemSlot::ItemSlot(uint32_t layer, uint32_t index, Inventory *inventory, InventoryContainer *container)
    : m_count(0), m_layer(layer), m_index(index), m_container(container), m_inventory(inventory)
{
    m_background = newref<ColorRect>();
    set_scale(glm::vec2(0.12));

    m_item_rect = newref<TextureRect>();
    m_label = newref<Label>(Engine::get().get_font());
}

void ItemSlot::update(float d)
{
    m_background->set_position(m_position);
    m_background->set_scale(m_scale);

    m_item_rect->set_position(m_position);
    m_item_rect->set_scale(m_scale * 0.9f);

    m_label->set_position(m_position + glm::vec2(0.01f, -0.03f));
    m_label->set_scale(m_scale * 0.5f);
    m_label->update(d);

    if (is_mouse_hovering())
    {
        m_background->set_color(Colors::red);
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
            bool allow_change = m_inventory->on_place(m_layer, m_count, grabbed.value(), m_container);

            if (!m_item.valid() && allow_change)
            {
                m_inventory->ungrab();
                m_container->set_stack(m_layer, m_index, grabbed.value());
            }
            else if (allow_change)
            {
                ItemStack stack = m_container->get_stack(m_layer, m_index);
                if (grabbed.value().item() == stack.item())
                {
                    Option<ItemStack> excess = stack.merge(grabbed.value());
                    m_container->set_stack(m_layer, m_index, stack);

                    if (excess.has_value())
                        m_inventory->grab(excess.value());
                    else
                        m_inventory->ungrab();
                }
            }
        }
        else if (m_item.valid())
        {
            bool allow_pick = m_inventory->on_pick(m_layer, m_index, ItemStack(m_item, m_count), m_container);
            if (allow_pick)
            {
                m_inventory->grab(ItemStack(m_item, m_count), InventoryOrigin(m_layer, m_index, m_container));
                m_container->set_stack(m_layer, m_index, ItemStack());
            }
        }
    }
}

void ItemSlot::process_event(Event& event)
{
    if (event.handled)
        return;
}

void ItemSlot::draw(WGPURenderPassEncoder encoder)
{
    m_background->draw(encoder);

    if (m_item.valid())
    {
        m_item_rect->draw(encoder);
        m_label->draw(encoder);
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
