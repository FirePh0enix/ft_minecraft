#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Item/ItemStack.hpp"
#include "UI/Container.hpp"
#include "UI/ItemSlot.hpp"

class Inventory;

class InventoryContainer : public Object
{
    CLASS(InventoryContainer, Object);

public:
    struct Layer
    {
        Vector<ItemStack> stacks;
    };

    Result<void> add_layer(size_t size);

    void set_stack(uint32_t layer, uint32_t i, ItemStack stack);

    ItemStack get_stack(uint32_t layer, uint32_t i) { return m_layers.get_unchecked(layer).stacks.get_unchecked(i); }
    const Layer& get_layer(uint32_t layer) const { return m_layers.get_unchecked(layer); }
    Layer& get_layer(uint32_t layer) { return m_layers.get_unchecked(layer); }
    Option<ItemStack> consume(Id<Item> item);

private:
    LocalVector<Layer> m_layers;
};

struct InventoryOrigin
{
    uint32_t layer = 0;
    uint32_t i;
    InventoryContainer *container;
};

class Inventory : public Container
{
    CLASS(Inventory, Container);

public:
    Inventory(Ref<InventoryContainer> container);

    virtual void update(float d) override;
    virtual void draw(const RenderPass& pass) override;

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

    void grab(const ItemStack& itemstack, Option<InventoryOrigin> origin = None);
    void ungrab();
    Option<ItemStack> get_grabbed();
    InventoryOrigin get_grabbed_origin();

    void grab_cancel();

    void add_grid(uint32_t w, uint32_t h, uint32_t layer, glm::vec2 pos = glm::vec2(), InventoryContainer *container = nullptr);
    void add_background();

protected:
    Ref<TextureRect> m_grabbed_item_rect;
    Ref<Label> m_grabbed_item_label;

    LocalVector<Vector<Ref<ItemSlot>>> m_grids;

    Option<ItemStack> m_grabbed_stack;
    InventoryOrigin m_grabbed_from;

    Ref<InventoryContainer> m_container;
};
