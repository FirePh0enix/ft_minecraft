#include "Inventory/CraftingTable.hpp"

#include "Core/Print.hpp"
#include "Engine.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/Item.hpp"

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

    if (layer == 1 && container == m_container.ptr())
        return false;

    if (layer == 0)
        m_dirty = true;

    return true;
}

bool CraftingTableInventory::on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
{
    (void)stack;
    (void)container;

    if (layer == 1 && index == 0)
    {
        ItemStack result = m_container->get_stack(1, 0);

        if (result.item().valid())
        {
            consume_ingredients();
            m_dirty = true;
        }
    }

    if (layer == 0)
        m_dirty = true;

    return true;
}

void CraftingTableInventory::update_recipe()
{
    InplaceVector<Id<Item>, 9> grid;
    for (size_t i = 0; i < 9; i++)
    {
        ItemStack s = m_container->get_stack(0, i);
        auto value = s.item().value > 0 ? s.item() : Id<Item>();
        grid.push_back(value);
    }

    Option<ItemStack> result = Engine::get().crafting().match(grid, 3, 3);
    if (result.has_value())
    {
        println("Matched item {}, count {}", result.value().item().value, result.value().count());
        m_container->set_stack(1, 0, result.value());
    }
    else
    {
        println("Not matching !");
        m_container->set_stack(1, 0, Id<Item>());
    }
}

void CraftingTableInventory::consume_ingredients()
{
    for (size_t i = 0; i < 9; i++)
    {
        ItemStack stack = m_container->get_stack(0, i);

        if (!stack.item().valid())
            continue;

        stack.set_count(stack.count() - 1);

        if (stack.count() <= 0)
            m_container->set_stack(0, i, ItemStack());
        else
            m_container->set_stack(0, i, stack);
    }
}