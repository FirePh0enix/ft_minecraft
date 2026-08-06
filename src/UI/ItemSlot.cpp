#include "UI/ItemSlot.hpp"

#include "Color.hpp"
#include "Engine.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"
#include "UI/Widget.hpp"
#include "World/Registry.hpp"

#include <cstddef>
#include <format>

ItemSlotWidget::ItemSlotWidget(uint32_t layer, uint32_t index, Inventory *inventory, InventoryContainer *container)
    : m_count(0), m_layer(layer), m_index(index), m_container(container), m_inventory(inventory)
{
    set_alignment(ContainerAlignment::Center);
    set_layout(ContainerLayout::Stack);

    m_background = std::make_shared<ColorRectWidget>();
    m_background->set_color(Colors::blue);
    m_background->set_size(Point(Size::px(80), Size::px(80)));
    add_child(m_background);

    m_item_rect = std::make_shared<TextureRectWidget>();
    m_item_rect->set_size(Point(Size::px(72), Size::px(72)));
    add_child(m_item_rect);

    m_label = std::make_shared<LabelWidget>(Engine::get().get_font());
    add_child(m_label);
}

void ItemSlotWidget::update(float d)
{
    Widget::update(d);

    if (is_mouse_hovering())
        m_background->set_color(Colors::red);
    else
        m_background->set_color(Colors::blue);

    if (is_mouse_hovering() && Input::is_action_just_pressed("ui_click"))
    {
        std::optional<ItemStack> grabbed = m_inventory->get_grabbed();
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
                    std::optional<ItemStack> excess = stack.merge(grabbed.value());
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
    else if (is_mouse_hovering() && Input::is_action_just_pressed("ui_rclick"))
    {
        std::optional<ItemStack> grabbed = m_inventory->get_grabbed();
        if (grabbed.has_value())
        {
            bool allow_change = m_inventory->on_place(m_layer, m_count, grabbed.value(), m_container);
            ItemStack gs = grabbed.value();
            if (allow_change && m_item.valid() && m_item == gs.item())
            {
                m_container->set_stack(m_layer, m_index, ItemStack(gs.item(), m_count + 1));
                gs.sub(1);
                m_inventory->grab(gs);
            }
            else if (allow_change && !m_item.valid())
            {
                m_container->set_stack(m_layer, m_index, ItemStack(gs.item(), 1));
                gs.sub(1);
                m_inventory->grab(gs);
            }
        }
    }
}

void ItemSlotWidget::process_event(Event& event)
{
    if (event.handled)
        return;
}

void ItemSlotWidget::draw(const RenderPass& pass)
{
    m_background->draw(pass);

    if (m_item.valid())
    {
        m_item_rect->draw(pass);
        m_label->draw(pass);
    }
}

void ItemSlotWidget::set_item(Id<Item> item)
{
    m_item = item;

    if (item.valid())
    {
        m_item_rect->set_texture(Engine::get().registry().get_texture(item));
        m_item_rect->set_visible(true);
    }
    else
    {
        m_item_rect->set_visible(false);
    }
}

void ItemSlotWidget::set_count(size_t count)
{
    std::string text = std::format("{}", count);
    m_label->set_text(text);
    m_count = count;
}
