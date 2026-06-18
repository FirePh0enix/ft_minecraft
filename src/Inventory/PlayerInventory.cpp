#include "Inventory/PlayerInventory.hpp"

#include "Color.hpp"
#include "Engine.hpp"
#include "Item/ItemStack.hpp"
#include "UI/ColorRect.hpp"
#include "UI/ItemSlot.hpp"

QuickSlot::QuickSlot()
    : m_count(0)
{
    m_background = newref<ColorRect>();
    set_scale(glm::vec2(0.12));

    m_item_rect = newref<TextureRect>();
    m_label = newref<Label>(Engine::get().get_font());
}

void QuickSlot::update(float d)
{
    m_background->set_position(m_position);
    m_background->set_scale(m_scale);

    m_item_rect->set_position(m_position);
    m_item_rect->set_scale(m_scale * 0.9f);

    m_label->set_position(m_position + glm::vec2(0.01f, -0.03f));
    m_label->set_scale(m_scale * 0.5f);
    m_label->update(d);

    if (m_selected)
    {
        m_background->set_color(Colors::yellow);
    }
    else
    {
        m_background->set_color(Colors::blue);
    }
}

void QuickSlot::draw(const RenderPass& pass)
{
    m_background->draw(pass);

    if (m_item.valid())
    {
        m_item_rect->draw(pass);
        m_label->draw(pass);
    }
}

void QuickSlot::set_item(Id<Item> item)
{
    m_item = item;

    if (item.valid())
    {
        Ref<Texture> texture = Engine::get().registry().get_texture(item);
        m_item_rect->set_texture(texture);
    }
}

void QuickSlot::set_count(size_t count)
{
    String text = format("{}", count);
    m_label->set_text(text);
    m_count = count;
}

PlayerInventory::PlayerInventory(Ref<InventoryContainer> container)
    : Inventory(container)
{
    add_background();
    add_grid(9, 3, 0, glm::vec2(0, -0.2f));
    add_grid(9, 1, 1, glm::vec2(0, -0.6f));

    add_grid(2, 2, 2, glm::vec2(-0.2f, 0.3f));
    add_grid(1, 1, 3, glm::vec2(0.3f, 0.3f));

    constexpr float slot_size = 0.12f;
    constexpr float slot_margin = 0.04f;
    constexpr float slot_tsize = slot_size + slot_margin;

    float offset_x = -(slot_tsize * inventory_width) / 2.0f + slot_tsize / 2.0f;
    float offset_y = -(slot_tsize * inventory_height) / 2.0f + slot_tsize / 2.0f;

    m_quick_slots_container = newref<Container>();
    for (size_t x = 0; x < inventory_width; x++)
    {
        Ref<QuickSlot> quick_slot = newref<QuickSlot>();
        quick_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, -slot_tsize * 4.0f));
        m_quick_slots_container->add_child(quick_slot);
        m_quick_slots[x] = quick_slot;
    }
}

void PlayerInventory::update(float d)
{
    Inventory::update(d);

    for (size_t x = 0; x < inventory_width; x++)
    {
        ItemStack stack = m_container->get_stack(1, x);
        if (!stack.item().valid())
        {
            m_quick_slots[x]->set_item(Id<Item>());
            continue;
        }
        m_quick_slots[x]->set_item(stack.item());
        m_quick_slots[x]->set_count(stack.count());
    }

    m_quick_slots_container->update(d);

    if (m_dirty)
    {
        update_recipe();
        m_dirty = false;
    }
}

void PlayerInventory::draw(const RenderPass& pass)
{
    Inventory::draw(pass);
}

void PlayerInventory::draw_toolbar(const RenderPass& pass)
{
    m_quick_slots_container->draw(pass);
}

void PlayerInventory::set_selected_slot(size_t slot)
{
    m_quick_slots[m_selected_slot]->set_selected(false);
    m_quick_slots[slot]->set_selected(true);
    m_selected_slot = slot;
}

void PlayerInventory::add_stack(ItemStack stack)
{
    for (size_t x = 0; x < inventory_width; x++)
    {
        ItemStack current_stack = m_container->get_stack(1, x);
        if (current_stack.item() == stack.item() && current_stack.count() < itemstack_max_size)
        {
            Option<ItemStack> excess = current_stack.merge(stack);
            m_container->set_stack(1, x, current_stack);

            if (excess.has_value())
                add_stack(excess.value());
            return;
        }
        else if (!current_stack.item().valid())
        {
            current_stack = stack;
            m_container->set_stack(1, x, current_stack);
            return;
        }
    }
    for (size_t i = 0; i < 27; i++)
    {
        ItemStack current_stack = m_container->get_stack(0, i);
        if (current_stack.item() == stack.item() && current_stack.count() < itemstack_max_size)
        {
            Option<ItemStack> excess = current_stack.merge(stack);
            m_container->set_stack(0, i, current_stack);

            if (excess.has_value())
                add_stack(excess.value());
            return;
        }
        else if (!current_stack.item().valid())
        {
            current_stack = stack;
            m_container->set_stack(0, i, current_stack);
            return;
        }
    }
}

bool PlayerInventory::on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)index;
    (void)stack;

    if (layer == 3 && container == m_container.ptr())
        return false;

    if (layer == 2)
        m_dirty = true;

    return true;
}

bool PlayerInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)stack;
    (void)container;

    if (layer == 3 && index == 0)
    {
        ItemStack result = m_container->get_stack(3, 0);

        if (result.item().valid())
        {
            consume_ingredients();
            m_dirty = true;
        }
    }

    if (layer == 2)
        m_dirty = true;

    return true;
}

void PlayerInventory::update_recipe()
{
    InplaceVector<Id<Item>, 9> grid;
    for (size_t i = 0; i < 9; i++)
    {
        ItemStack s = m_container->get_stack(2, i);
        auto value = s.item().valid() ? s.item() : Id<Item>();
        grid.push_back(value);
    }

    Option<ItemStack> result = Engine::get().crafting().match(grid, 2, 2);
    if (result.has_value())
    {
        println("Matched item {}, count {}", result.value().item().value, result.value().count());
        m_container->set_stack(3, 0, result.value());
    }
    else
    {
        println("Not matching !");
        m_container->set_stack(3, 0, ItemStack());
    }
}

void PlayerInventory::consume_ingredients()
{
    for (size_t i = 0; i < 4; i++)
    {
        ItemStack stack = m_container->get_stack(2, i);

        if (!stack.item().valid())
            continue;

        stack.set_count(stack.count() - 1);

        if (stack.count() <= 0)
            m_container->set_stack(2, i, ItemStack());
        else
            m_container->set_stack(2, i, stack);
    }
}