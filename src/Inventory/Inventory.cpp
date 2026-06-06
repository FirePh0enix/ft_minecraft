#include "Inventory/Inventory.hpp"

#include "Engine.hpp"

Result<void> InventoryContainer::add_layer(size_t size)
{
    Vector<ItemStack> stacks;
    stacks.resize(size);
    m_layers.append(Layer(stacks));
    return Result<void>();
}

void InventoryContainer::set_stack(uint32_t layer, uint32_t i, ItemStack stack)
{
    m_layers[layer].stacks[i] = stack;
}

Inventory::Inventory(Ref<InventoryContainer> container)
    : m_container(container)
{
    m_grabbed_item_rect = newref<TextureRect>();
    m_grabbed_item_rect->set_scale(glm::vec2(0.12) * 0.9f);

    m_grabbed_item_label = newref<Label>(Engine::get().get_font());
    m_grabbed_item_label->set_scale(glm::vec2(0.12) * 0.8f);
}

void Inventory::update(float d)
{
    Container::update(d);

    for (const auto& slots : m_grids)
    {
        for (Ref<ItemSlot> is : slots)
        {
            ItemStack stack = is->container()->get_stack(is->layer(), is->index());
            if (!stack.item().valid())
            {
                is->set_item(Id<Item>());
                continue;
            }
            is->set_item(stack.item());
            is->set_count(stack.count());
        }
    }

    if (get_grabbed().has_value())
    {
        m_grabbed_item_label->update(d);
    }
}

void Inventory::draw(const RenderPassNode& node)
{
    Container::draw(node);

    if (m_grabbed_stack.has_value())
    {
        m_grabbed_item_rect->set_position(Input::get_mouse_absolute());
        m_grabbed_item_label->set_position(Input::get_mouse_absolute());

        m_grabbed_item_rect->draw(node);
        m_grabbed_item_label->draw(node);
    }
}

void Inventory::grab(const ItemStack& itemstack, Option<InventoryOrigin> pos)
{
    m_grabbed_stack = itemstack;
    if (pos.has_value())
        m_grabbed_from = pos.value_or(InventoryOrigin());

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

InventoryOrigin Inventory::get_grabbed_origin()
{
    return m_grabbed_from;
}

void Inventory::grab_cancel()
{
    if (m_grabbed_stack.has_value())
        m_grabbed_from.container->set_stack(m_grabbed_from.layer, m_grabbed_from.i, m_grabbed_stack.value());
}

void Inventory::add_grid(uint32_t w, uint32_t h, uint32_t layer, glm::vec2 pos, InventoryContainer *container)
{
    if (container == nullptr)
    {
        container = m_container.ptr();
    }

    Vector<Ref<ItemSlot>> slots;
    slots.reserve(w * h);

    constexpr float slot_size = 0.12f;
    constexpr float slot_margin = 0.04f;
    constexpr float slot_tsize = slot_size + slot_margin;

    float offset_x = -(slot_tsize * float(w)) / 2.0f + slot_tsize / 2.0f + pos.x;
    float offset_y = -(slot_tsize * float(h)) / 2.0f + slot_tsize / 2.0f + pos.y;
    for (size_t x = 0; x < w; x++)
        for (size_t y = 0; y < h; y++)
        {
            Ref<ItemSlot> item_slot = newref<ItemSlot>(layer, x + y * w, this, container);
            item_slot->set_position(glm::vec2(offset_x, offset_y) + glm::vec2(float(x) * slot_tsize, float(y) * slot_tsize));
            add_child(item_slot);
            slots.append(item_slot);
        }

    m_grids.append(slots);
}

void Inventory::add_background()
{
    Ref<ColorRect> background = newref<ColorRect>();
    background->set_scale(glm::vec2(2.5, 1.5));
    background->set_color(Color(0.15));
    add_child(background);
}
