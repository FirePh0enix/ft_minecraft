#include "Inventory/Inventory.hpp"

#include "Engine.hpp"
#include "UI/Widget.hpp"

#include <format>

void InventoryContainer::add_layer(size_t size)
{
    std::vector<ItemStack> stacks;
    stacks.resize(size);
    m_layers.push_back(Layer(stacks));
}

void InventoryContainer::set_stack(uint32_t layer, uint32_t i, ItemStack stack)
{
    m_layers[layer].stacks[i] = stack;
}

std::optional<ItemStack> InventoryContainer::consume(Id<Item> item)
{
    // Quick Inventory + normal inventory;
    for (size_t layer_index : {1, 0})
    {
        auto& layer = m_layers[layer_index];
        for (size_t i = 0; i < layer.stacks.size(); i++)
        {
            ItemStack& stack = layer.stacks[i];

            if (!stack.item().valid() || stack.item() != item)
                continue;

            ItemStack consumed(item, 1);
            stack.sub(1);
            return consumed;
        }
    }

    return {};
}

Inventory::Inventory(std::shared_ptr<InventoryContainer> container)
    : m_container(container)
{
    set_layout(ContainerLayout::Stack);
    set_expand_vertical(true);
    set_expand_horizontal(true);

    m_grabbed_item_rect = std::make_shared<TextureRectWidget>();
    m_grabbed_item_rect->set_size(Point(Size::px(90), Size::px(90)));

    m_grabbed_item_label = std::make_shared<LabelWidget>(Engine::get().get_font());
    // m_grabbed_item_label->set_scale(glm::vec2(0.12) * 0.8f);
    m_grabbed_item_rect->add_child(m_grabbed_item_label);

    add_child(m_grabbed_item_rect);
}

void Inventory::update(float d)
{
    Widget::update(d);

    for (const auto& slots : m_grids)
    {
        for (std::shared_ptr<ItemSlotWidget> is : slots)
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
}

void Inventory::grab(const ItemStack& itemstack, std::optional<InventoryOrigin> pos)
{
    if (itemstack.count() == 0 || !itemstack.item().valid())
    {
        ungrab();
        return;
    }

    m_grabbed_stack = itemstack;
    if (pos.has_value())
        m_grabbed_from = pos.value_or(InventoryOrigin());

    m_grabbed_item_rect->set_texture(Engine::get().registry().get_texture(itemstack.item()));
    m_grabbed_item_label->set_text(std::format("{}", itemstack.count()));
}

void Inventory::ungrab()
{
    m_grabbed_stack = std::nullopt;
}

std::optional<ItemStack> Inventory::get_grabbed()
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
    {
        m_grabbed_from.container->set_stack(m_grabbed_from.layer, m_grabbed_from.i, m_grabbed_stack.value());
        m_grabbed_stack = std::nullopt;
    }
}

void Inventory::add_grid(uint32_t w, uint32_t h, uint32_t layer, Point offset, InventoryContainer *container)
{
    if (container == nullptr)
    {
        container = m_container.get();
    }

    std::vector<std::shared_ptr<ItemSlotWidget>> slots;
    slots.reserve(w * h);

    std::shared_ptr<Widget> super_container = std::make_shared<Widget>();
    super_container->set_size(Point(Size::percent(100.0), Size::percent(100.0)));
    super_container->set_alignment(ContainerAlignment::Center);

    std::shared_ptr<Widget> grid_container = std::make_shared<Widget>();
    grid_container->set_layout(ContainerLayout::Vertical);
    grid_container->set_spacing(Point(Size::px(0), Size::px(10)));
    grid_container->set_offset(offset);
    super_container->add_child(grid_container);

    for (int32_t y = 0; y < int32_t(h); y++)
    {
        std::shared_ptr<Widget> line_container = std::make_shared<Widget>();
        line_container->set_spacing(Point(Size::px(10), Size::px(0)));

        for (int32_t x = 0; x < int32_t(w); x++)
        {
            std::shared_ptr<ItemSlotWidget> item_slot = std::make_shared<ItemSlotWidget>(layer, x + y * w, this, container);
            line_container->add_child(item_slot);
            slots.push_back(item_slot);
        }

        grid_container->add_child(line_container);
    }

    m_grids.push_back(slots);
    add_child(super_container);
}

void Inventory::add_background()
{
    std::shared_ptr<ColorRectWidget> background = std::make_shared<ColorRectWidget>();
    background->set_color(Color(0.15));
    add_child(background);
}
