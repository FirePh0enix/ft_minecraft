#pragma once

#include "Inventory/Inventory.hpp"

class CraftingTableInventory : public Inventory
{
    CLASS(CraftingTableInventory, Inventory);

public:
    CraftingTableInventory(Ref<InventoryContainer> inventory, Ref<InventoryContainer> player_inventory);

    virtual void update(float d) override;
    virtual void draw(const RenderPass& pass) override;
    virtual void process_event(Event& event) override { (void)event; }

    virtual bool on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container) override;
    virtual bool on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container) override;

    void update_recipe();
    void consume_ingredients();

private:
    Ref<InventoryContainer> m_player_inventory;
    bool m_dirty = false;
};
