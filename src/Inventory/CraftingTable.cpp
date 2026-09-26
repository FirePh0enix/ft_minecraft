#include "Inventory/CraftingTable.hpp"

#include "Engine.hpp"
#include "Entity/Player.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/Item.hpp"
#include "World/Registry.hpp"

#include <algorithm>

constexpr int CRAFTING_GRID_SIZE = 9;
constexpr int INGREDIENTS_LAYER = 0;
constexpr int RESULT_LAYER = 1;

CraftingTableInventory::CraftingTableInventory(std::shared_ptr<InventoryContainer> inventory, std::shared_ptr<InventoryContainer> player_inventory, Player *player)
    : Inventory(inventory), m_player_inventory(player_inventory), m_player(player)
{
    std::shared_ptr<TextureRectWidget> background = std::make_shared<TextureRectWidget>();

    background->set_texture(Texture::load("data/resourcepacks/core/assets/minecraft/textures/gui/container/crafting_table.png", 176, 166).value_or(Renderer::get().get_missing_texture()));
    background->set_size(Point(Size::px(176 * 4), Size::px(166 * 4)));
    add_child(background);

    add_grid(9, 3, 0, Point(Size::px(0), Size::px(108)), m_player_inventory.get());
    add_grid(9, 1, 1, Point(Size::px(0), Size::px(268)), m_player_inventory.get());

    add_grid(3, 3, 0, Point(Size::px(-130), Size::px(-160)));
    add_grid(1, 1, 1, Point(Size::px(175), Size::px(-160)));

    add_child(m_grabbed_item_rect); // NOTE: quick hack to draw the grabbed item on top
}

void CraftingTableInventory::update(float d)
{
    Inventory::update(d);
    if (m_dirty)
    {
        update_recipe();
        m_dirty = false;
    }
}

void CraftingTableInventory::draw(const RenderPass& pass)
{
    Inventory::draw(pass);
}

bool CraftingTableInventory::on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)index;
    (void)stack;

    if (container == m_container.get() && layer == RESULT_LAYER)
        return false;

    if (container == m_container.get() && layer == INGREDIENTS_LAYER)
        m_dirty = true;

    return true;
}

bool CraftingTableInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)stack;
    (void)container;

    if (container == m_container.get() && layer == RESULT_LAYER && index == 0)
    {
        ItemStack result = m_container->get_stack(RESULT_LAYER, 0);

        if (result.item().valid())
        {
            consume_ingredients();
            m_dirty = true;
        }
    }

    if (container == m_container.get() && layer == INGREDIENTS_LAYER)
        m_dirty = true;

    return true;
}

void CraftingTableInventory::on_change(InventoryContainer *container)
{
    if (container == m_player_inventory.get())
        m_player->sync_inventory();
}

bool CraftingTableInventory::on_close()
{
    // Prepare the full transfer before changing either inventory.
    auto main = m_player_inventory->get_layer(0).stacks;
    auto toolbar = m_player_inventory->get_layer(1).stacks;

    for (const ItemStack& ingredient : m_container->get_layer(INGREDIENTS_LAYER).stacks)
    {
        if (!ingredient.item().valid() || ingredient.count() == 0)
            continue;

        size_t remaining = ingredient.count();
        for (auto *slots : {&toolbar, &main})
            for (ItemStack& slot : *slots)
            {
                if (remaining == 0)
                    break;
                if (slot.item() != ingredient.item() || slot.get_tags() != ingredient.get_tags() || slot.count() >= itemstack_max_size)
                    continue;
                const size_t moved = std::min(remaining, itemstack_max_size - slot.count());
                slot.set_count(slot.count() + moved);
                remaining -= moved;
            }

        for (auto *slots : {&toolbar, &main})
            for (ItemStack& slot : *slots)
            {
                if (remaining == 0)
                    break;
                if (slot.item().valid() && slot.count() != 0)
                    continue;
                const size_t moved = std::min(remaining, itemstack_max_size);
                slot = ingredient;
                slot.set_count(moved);
                remaining -= moved;
            }

        if (remaining != 0)
            return false;
    }

    for (size_t i = 0; i < main.size(); ++i)
        m_player_inventory->set_stack(0, i, main[i]);
    for (size_t i = 0; i < toolbar.size(); ++i)
        m_player_inventory->set_stack(1, i, toolbar[i]);
    m_player->sync_inventory();
    return true;
}

void CraftingTableInventory::update_recipe()
{
    std::array<Id<Item>, MAX_RECIPE_SIZE> grid;
    for (size_t i = 0; i < CRAFTING_GRID_SIZE; i++)
    {
        ItemStack s = m_container->get_stack(INGREDIENTS_LAYER, i);
        grid[i] = s.item();
    }

    std::optional<ItemStack> result = Engine::get().registry().match(grid, 3, 3);
    if (result.has_value())
        m_container->set_stack(RESULT_LAYER, 0, result.value());
    else
        m_container->set_stack(RESULT_LAYER, 0, Id<Item>());
}

void CraftingTableInventory::consume_ingredients()
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
