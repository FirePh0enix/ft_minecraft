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

    constexpr size_t inventory_width = 9;
    constexpr size_t inventory_height = 5;
    constexpr float slot_size = 0.12f;
    constexpr float slot_margin = 0.04f;
    constexpr float slot_tsize = slot_size + slot_margin;

    float offset_x = -(slot_tsize * inventory_width) / 2.0f + slot_tsize / 2.0f;
    float offset_y = -(slot_tsize * inventory_height) / 2.0f + slot_tsize / 2.0f;
    for (size_t x = 0; x < inventory_width; x++)
        for (size_t y = 0; y < inventory_height; y++)
        {
            Ref<ItemSlot> item_slot = EXPECT(newref<ItemSlot>());
            item_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, float(y) * slot_tsize));
            EXPECT(m_slots_container->add_child(item_slot));

            m_slots[x + y * 9] = item_slot;
        }

    for (size_t x = 0; x < inventory_width; x++)
    {
        Ref<ItemSlot> item_slot = EXPECT(newref<ItemSlot>());
        item_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, -slot_tsize * 1.5f));
        EXPECT(m_quick_slots_container->add_child(item_slot));

        m_quick_slots[x] = item_slot;
    }
}

void Inventory::update(float d)
{
    for (size_t x = 0; x < 9; x++)
        for (size_t y = 0; y < 5; y++)
        {
            ItemStack stack = m_data[x + y * 9];
            if (stack.get_block() == 0)
            {
                m_slots[x + y * 9]->set_empty(true);
                continue;
            }
            Ref<Block> block = Engine::get().blocks().get_block_by_id(stack.get_block());
            m_slots[x + y * 9]->set_texture(Engine::get().blocks().get_texture(block->get_texture_index(Axis::Z, true)));
            m_slots[x + y * 9]->set_count(stack.count());
        }
    for (size_t x = 0; x < 9; x++)
    {
        ItemStack stack = m_quick_data[x];
        if (stack.get_block() == 0)
        {
            m_quick_slots[x]->set_empty(true);
            continue;
        }
        Ref<Block> block = Engine::get().blocks().get_block_by_id(stack.get_block());
        m_quick_slots[x]->set_texture(Engine::get().blocks().get_texture(block->get_texture_index(Axis::Z, true)));
        m_quick_slots[x]->set_count(stack.count());
    }

    if (m_open)
        m_slots_container->update(d);
    m_quick_slots_container->update(d);
}

void Inventory::process_event(Event& event)
{
    (void)event;
}

void Inventory::draw(const RenderPassNode& node)
{
    if (m_open)
        m_slots_container->draw(node);
    m_quick_slots_container->draw(node);
}

void Inventory::set_selected_slot(size_t slot)
{
    m_quick_slots[m_selected_slot]->set_selected(false);
    m_quick_slots[slot]->set_selected(true);
    m_selected_slot = slot;
}

void Inventory::add_stack(ItemStack stack)
{
    for (size_t x = 0; x < 9; x++)
    {
        if (m_quick_data[x].get_block() == stack.get_block())
        {
            m_quick_data[x].set_count(m_quick_data[x].count() + stack.count());
            return;
        }
        else if (m_quick_data[x].get_block() == 0)
        {
            m_quick_data[x] = stack;
            return;
        }
    }

    // TODO and the rest
}
