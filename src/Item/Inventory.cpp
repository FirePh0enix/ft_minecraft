#include "Item/Inventory.hpp"

#include "Engine.hpp"
#include "UI/ColorRect.hpp"
#include "UI/ItemSlot.hpp"

Inventory::Inventory()
{
    m_slots_container = EXPECT(newref<Container>());
    m_quick_slots_container = EXPECT(newref<Container>());

    Ref<ColorRect> background = EXPECT(newref<ColorRect>());
    background->set_scale(glm::vec2(2.5, 1.5));
    background->set_color(Color(0.15));
    EXPECT(m_slots_container->add_child(background));

    constexpr float slot_size = 0.12f;
    constexpr float slot_margin = 0.04f;
    constexpr float slot_tsize = slot_size + slot_margin;

    float offset_x = -(slot_tsize * inventory_width) / 2.0f + slot_tsize / 2.0f;
    float offset_y = -(slot_tsize * inventory_height) / 2.0f + slot_tsize / 2.0f;
    for (size_t x = 0; x < inventory_width; x++)
        for (size_t y = 0; y < inventory_height; y++)
        {
            Ref<ItemSlot> item_slot = EXPECT(newref<ItemSlot>(glm::i64vec2(x, y), this));
            item_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, float(y) * slot_tsize));
            EXPECT(m_slots_container->add_child(item_slot));

            m_slots[x + y * inventory_width] = item_slot;
        }

    for (size_t x = 0; x < inventory_width; x++)
    {
        Ref<ItemSlot> item_slot = EXPECT(newref<ItemSlot>(glm::i64vec2(x, inventory_height), this));
        item_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, -slot_tsize * 1.5f));
        EXPECT(m_quick_slots_container->add_child(item_slot));

        m_slots[x + inventory_height * inventory_width] = item_slot;
    }

    m_grabbed_item_rect = EXPECT(newref<TextureRect>());
    m_grabbed_item_rect->set_scale(glm::vec2(0.12) * 0.9f);

    m_grabbed_item_label = EXPECT(newref<Label>(Engine::get().get_font()));
    m_grabbed_item_label->set_scale(glm::vec2(0.12) * 0.8f);
}

void Inventory::update(float d)
{
    for (size_t x = 0; x < inventory_width; x++)
        for (size_t y = 0; y < inventory_height; y++)
        {
            ItemStack stack = m_data[x + y * inventory_width];
            if (!stack.item().valid())
            {
                m_slots[x + y * inventory_width]->set_item(Id<Item>());
                continue;
            }
            m_slots[x + y * inventory_width]->set_item(stack.item());
            m_slots[x + y * inventory_width]->set_count(stack.count());
        }
    for (size_t x = 0; x < inventory_width; x++)
    {
        ItemStack stack = quick_stack(x);
        if (!stack.item().valid())
        {
            m_slots[x + inventory_height * inventory_width]->set_item(Id<Item>());
            continue;
        }
        m_slots[x + inventory_height * inventory_width]->set_item(stack.item());
        m_slots[x + inventory_height * inventory_width]->set_count(stack.count());
    }

    if (m_open)
        m_slots_container->update(d);
    m_quick_slots_container->update(d);

    if (m_open && m_grabbed_stack.has_value())
    {
        m_grabbed_item_label->update(d);
    }
}

void Inventory::grab(const ItemStack& itemstack, glm::i64vec2 pos)
{
    m_grabbed_stack = itemstack;
    if (pos != glm::i64vec2(-1))
        m_grabbed_from = pos;

    m_grabbed_item_rect->set_texture(Engine::get().registry().get_texture(itemstack.item()));
    m_grabbed_item_label->set_text(format("{}", itemstack.count()));
}

void Inventory::ungrab()
{
    m_grabbed_stack = None;
}

Option<ItemStack> Inventory::get_grabbed()
{
    return m_grabbed_stack;
}

void Inventory::process_event(Event& event)
{
    (void)event;
}

void Inventory::draw(const RenderPassNode& node)
{
    if (m_open)
    {
        m_slots_container->draw(node);
    }

    m_quick_slots_container->draw(node);

    if (m_open && m_grabbed_stack.has_value())
    {
        m_grabbed_item_rect->set_position(Input::get_mouse_absolute());
        m_grabbed_item_label->set_position(Input::get_mouse_absolute());

        m_grabbed_item_rect->draw(node);
        m_grabbed_item_label->draw(node);
    }
}

void Inventory::set_selected_slot(size_t slot)
{
    m_slots[m_selected_slot + inventory_height * inventory_width]->set_selected(false);
    m_slots[slot + inventory_height * inventory_width]->set_selected(true);
    m_selected_slot = slot;
}

void Inventory::set_open(bool v)
{
    m_open = v;

    if (!m_open && m_grabbed_stack.has_value())
    {
        stackxy(m_grabbed_from) = ItemStack(m_grabbed_stack.get().item(), m_grabbed_stack.get().count());
        ungrab();
    }
}

void Inventory::add_stack(ItemStack stack)
{
    for (size_t x = 0; x < inventory_width; x++)
    {
        if (quick_stack(x).item() == stack.item())
        {
            quick_stack(x).set_count(quick_stack(x).count() + stack.count());
            return;
        }
        else if (!quick_stack(x).item().valid())
        {
            quick_stack(x) = stack;
            return;
        }
    }

    // TODO and the rest
}
