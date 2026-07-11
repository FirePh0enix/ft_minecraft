#include "Inventory/CraftingTable.hpp"

#include "Engine.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/Item.hpp"
#include "World/Registry.hpp"


constexpr int CRAFTING_GRID_SIZE = 9;
constexpr int INGREDIENTS_LAYER = 0;
constexpr int RESULT_LAYER = 1;

CraftingTableInventory::CraftingTableInventory(Ref<InventoryContainer> inventory, Ref<InventoryContainer> player_inventory)
    : Inventory(inventory), m_player_inventory(player_inventory)
{
    add_background();
    add_grid(9, 3, 0, glm::vec2(0, -0.2f), m_player_inventory.ptr());
    add_grid(9, 1, 1, glm::vec2(0, -0.6f), m_player_inventory.ptr());

    add_grid(3, 3, 0, glm::vec2(-0.2f, 0.3f));
    add_grid(1, 1, 1, glm::vec2(0.3f, 0.3f));
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

    if (layer == RESULT_LAYER && container == m_container.ptr())
        return false;

    if (layer == INGREDIENTS_LAYER)
        m_dirty = true;

    return true;
}

bool CraftingTableInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
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

void CraftingTableInventory::update_recipe()
{
    InplaceVector<Id<Item>, RECIPE_SIZE> grid;
    for (size_t i = 0; i < CRAFTING_GRID_SIZE; i++)
    {
        ItemStack s = m_container->get_stack(INGREDIENTS_LAYER, i);
        grid.push_back(s.item());
    }

    Option<ItemStack> result = Engine::get().registry().match(grid, 3, 3);
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