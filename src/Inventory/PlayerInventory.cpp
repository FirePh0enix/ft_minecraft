#include "Inventory/PlayerInventory.hpp"

#include "Color.hpp"
#include "Engine.hpp"
#include "Item/ItemStack.hpp"
#include "UI/ItemSlot.hpp"
#include "UI/Widget.hpp"
#include "World/Registry.hpp"

#include <format>

constexpr int CRAFTING_GRID_SIZE = 4;
constexpr int INGREDIENTS_LAYER = 2;
constexpr int RESULT_LAYER = 3;

QuickSlotWidget::QuickSlotWidget()
    : m_count(0)
{
    set_alignment(ContainerAlignment::Center);
    set_layout(ContainerLayout::Stack);

    m_background = std::make_shared<ColorRectWidget>();
    m_background->set_color(Colors::blue);
    m_background->set_size(Point(Size::px(80), Size::px(80)));
    add_child(m_background);

    m_item_rect = std::make_shared<TextureRectWidget>();
    m_item_rect->set_size(Point(Size::px(72), Size::px(72)));
    m_item_rect->set_visible(false);
    add_child(m_item_rect);

    m_label = std::make_shared<LabelWidget>(Engine::get().get_font());
    add_child(m_label);
}

void QuickSlotWidget::update(float d)
{
    Widget::update(d);

    if (m_selected)
    {
        m_background->set_color(Colors::yellow);
    }
    else
    {
        m_background->set_color(Colors::blue);
    }
}

void QuickSlotWidget::draw(const RenderPass& pass)
{
    m_background->draw(pass);

    if (m_item.valid())
    {
        m_item_rect->draw(pass);
        m_label->draw(pass);
    }
}

void QuickSlotWidget::set_item(Id<Item> item)
{
    m_item = item;

    if (item.valid())
    {
        std::shared_ptr<Texture> texture = Engine::get().registry().get_texture(item);
        m_item_rect->set_texture(texture);
        m_item_rect->set_visible(true);
    }
    else
    {
        m_item_rect->set_visible(false);
    }
}

void QuickSlotWidget::set_count(size_t count)
{
    std::string text = std::format("{}", count);
    m_label->set_text(text);
    m_count = count;
}

PlayerInventory::PlayerInventory(std::shared_ptr<InventoryContainer> container)
    : Inventory(container)
{
    add_background();
    add_grid(9, 3, 0, Point(Size::px(0), Size::px(40)));
    add_grid(9, 1, 1, Point(Size::px(0), Size::px(300)));

    add_grid(2, 2, 2, Point(Size::px(60), Size::px(-200)));
    add_grid(1, 1, 3, Point(Size::px(240), Size::px(-200)));

    m_quick_slots_container = std::make_shared<Widget>();
    m_quick_slots_container->set_layout(ContainerLayout::Horizontal);
    m_quick_slots_container->set_alignment(ContainerAlignment::CenterX | ContainerAlignment::Bottom);
    m_quick_slots_container->set_expand_horizontal(true);
    m_quick_slots_container->set_expand_vertical(true);

    std::shared_ptr<Widget> subcontainer = std::make_shared<Widget>();

    for (int32_t x = 0; x < int32_t(inventory_width); x++)
    {
        std::shared_ptr<QuickSlotWidget> quick_slot = std::make_shared<QuickSlotWidget>();

        subcontainer->add_child(quick_slot);
        m_quick_slots[x] = quick_slot;
    }

    m_quick_slots_container->add_child(subcontainer);
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

    if (m_dirty)
    {
        update_recipe();
        m_dirty = false;
    }
}

void PlayerInventory::draw_toolbar(const RenderPass& pass)
{
    m_quick_slots_container->draw_everything(pass);
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
            std::optional<ItemStack> excess = current_stack.merge(stack);
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
            std::optional<ItemStack> excess = current_stack.merge(stack);
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

    if (layer == RESULT_LAYER && container == m_container.get())
        return false;

    if (layer == INGREDIENTS_LAYER)
        m_dirty = true;

    return true;
}

bool PlayerInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)stack;
    (void)container;

    if (layer == RESULT_LAYER && index == 0)
    {
        ItemStack result = m_container->get_stack(RESULT_LAYER, 0);

        if (result.item().valid())
        {
            consume_ingredients();
            m_dirty = true;
        }
    }

    if (layer == INGREDIENTS_LAYER)
        m_dirty = true;

    return true;
}

void PlayerInventory::update_recipe()
{
    std::array<Id<Item>, MAX_RECIPE_SIZE> grid;
    for (size_t i = 0; i < CRAFTING_GRID_SIZE; i++)
    {
        ItemStack s = m_container->get_stack(INGREDIENTS_LAYER, i);
        grid[i] = s.item();
    }

    std::optional<ItemStack> result = Engine::get().registry().match(grid, 2, 2);
    if (result.has_value())
        m_container->set_stack(RESULT_LAYER, 0, result.value());
    else
        m_container->set_stack(RESULT_LAYER, 0, ItemStack());
}

void PlayerInventory::consume_ingredients()
{
    for (size_t i = 0; i < CRAFTING_GRID_SIZE; i++)
    {
        ItemStack stack = m_container->get_stack(INGREDIENTS_LAYER, i);

        if (!stack.item().valid())
            continue;

        stack.set_count(stack.count() - 1);

        if (stack.count() <= 0)
            m_container->set_stack(INGREDIENTS_LAYER, i, ItemStack());
        else
            m_container->set_stack(INGREDIENTS_LAYER, i, stack);
    }
}
