#pragma once

#include "Core/Class.hpp"
#include "Item/ItemStack.hpp"
#include "UI/ItemSlot.hpp"

class Inventory;

class InventoryContainer : public Object
{
    CLASS(InventoryContainer, Object);

public:
    struct Layer
    {
        std::vector<ItemStack> stacks;
    };

    Result<void> add_layer(size_t size);

    void set_stack(uint32_t layer, uint32_t i, ItemStack stack);

    ItemStack get_stack(uint32_t layer, uint32_t i) { return m_layers[layer].stacks[i]; }
    const Layer& get_layer(uint32_t layer) const { return m_layers[layer]; }
    Layer& get_layer(uint32_t layer) { return m_layers[layer]; }

private:
    std::vector<Layer> m_layers;
};

struct InventoryOrigin
{
    uint32_t layer = 0;
    uint32_t i;
    InventoryContainer *container;
};

class Inventory : public Widget
{
    CLASS(Inventory, Widget);

public:
    Inventory(std::shared_ptr<InventoryContainer> container);

    virtual void update(float d) override;

    /**
     * Callback for when a itemstack is placed into a slot. This functions is called before modifying the content of the
     * inventory.
     * @returns `false` can be returned to prevent the item from going to this slot.
     */
    virtual bool on_place(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
    {
        (void)layer;
        (void)index;
        (void)stack;
        (void)container;
        return true;
    }

    /**
     * Callback for when a itemstack is picked from a slot. This functions is called before modifying the content of the
     * inventory.
     * @returns `false` can be returned to prevent the item from going to this slot.
     */
    virtual bool on_pick(uint32_t layer, uint32_t index, ItemStack stack, InventoryContainer *container)
    {
        (void)layer;
        (void)index;
        (void)stack;
        (void)container;
        return true;
    }

    void grab(const ItemStack& itemstack, std::optional<InventoryOrigin> origin = std::nullopt);
    void ungrab();
    std::optional<ItemStack> get_grabbed();
    InventoryOrigin get_grabbed_origin();

    void grab_cancel();

    void add_grid(uint32_t w, uint32_t h, uint32_t layer, Point offset = Point(), InventoryContainer *container = nullptr);
    void add_background();

protected:
    std::shared_ptr<TextureRectWidget> m_grabbed_item_rect;
    std::shared_ptr<LabelWidget> m_grabbed_item_label;

    std::vector<std::vector<std::shared_ptr<ItemSlotWidget>>> m_grids;

    std::optional<ItemStack> m_grabbed_stack;
    InventoryOrigin m_grabbed_from;

    std::shared_ptr<InventoryContainer> m_container;
};
