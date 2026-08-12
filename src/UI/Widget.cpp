#include "Widget.hpp"

#include "Core/Types.hpp"
#include "Engine.hpp"
#include "Variant.hpp"

#include <cmath>
#include <memory>

void Widget::bind_static()
{
}

static Variant lerp(const Variant& a, const Variant& b, float t)
{
    if (a.tag != b.tag)
        return nullptr;

    switch (a.tag)
    {
    case VariantType::Color:
    {
        Color ca = a.get_unchecked<Color>();
        Color cb = b.get_unchecked<Color>();

        Color cr;
        cr.r = std::lerp(ca.r, cb.r, t);
        cr.g = std::lerp(ca.g, cb.g, t);
        cr.b = std::lerp(ca.b, cb.b, t);
        cr.a = std::lerp(ca.a, cb.a, t);
        return cr;
    }
    break;
    default:
        break;
    }

    return nullptr;
}

void Widget::update(float delta)
{
    std::set<uint64_t> ids;
    for (RuntimeAnimation& anim : m_runtime_animations)
    {
        if (anim.animation->_transition == TransitionType::Lerp)
        {
            Variant v = lerp(anim.original_value, anim.animation->_value, anim.timer / anim.animation->_time);
            set(anim.animation->_property, v);
        }

        anim.timer += delta;

        if (anim.timer >= anim.animation->_time)
        {
            if (anim.animation->_transition == TransitionType::Set)
                set(anim.animation->_property, anim.animation->_value);

            ids.insert(anim.id);
        }
    }

    for (uint64_t id : ids)
    {
        m_runtime_animations.erase(std::find_if(m_runtime_animations.begin(), m_runtime_animations.end(), [id](const auto& a)
                                                { return a.id == id; }));
    }

    if (is_mouse_hovering() && !m_was_hovering)
    {
        cancel_animations(AnimationType::HoverOut, false);
        for (const std::shared_ptr<Animation>& anim : m_animations[AnimationType::HoverIn])
            m_runtime_animations.push_back(RuntimeAnimation{.type = AnimationType::HoverIn, .animation = anim, .original_value = get(anim->_property), .id = new_anim_id()});

        m_was_hovering = true;
    }

    if (!is_mouse_hovering() && m_was_hovering)
    {
        cancel_animations(AnimationType::HoverIn, false);
        for (const std::shared_ptr<Animation>& anim : m_animations[AnimationType::HoverOut])
            m_runtime_animations.push_back(RuntimeAnimation{.type = AnimationType::HoverOut, .animation = anim, .original_value = get(anim->_property), .id = new_anim_id()});

        m_was_hovering = false;
    }
}

void Widget::add_child(std::shared_ptr<Widget> widget)
{
    widget->set_parent(this);
    m_children.push_back(widget);
}

GlobalPoint Widget::get_global_pos() const
{
    GlobalPoint p{};

    if (m_parent != nullptr)
    {
        size_t index = m_parent->get_child_index(this);
        GlobalPoint parent_pos = m_parent->get_global_pos();
        GlobalPoint parent_size = m_parent->get_global_size();
        GlobalPoint parent_spacing = m_parent->globalize(m_parent->get_spacing());

        GlobalPoint self_size = get_global_size();
        GlobalPoint self_offset = globalize(get_offset());

        if (index > 0)
        {
            std::shared_ptr<Widget> sibling = m_parent->m_children[index - 1];
            GlobalPoint pos = sibling->get_global_pos();
            GlobalPoint size = sibling->get_global_size();

            switch (m_parent->m_layout)
            {
            case ContainerLayout::Vertical:
                p.x = parent_pos.x + self_offset.x;
                p.y = pos.y + size.y + self_offset.y;
                break;
            case ContainerLayout::Horizontal:
                p.x = pos.x + size.x + self_offset.x;
                p.y = parent_pos.y + self_offset.y;
                break;
            case ContainerLayout::Stack:
                p.x = parent_pos.x + self_offset.x;
                p.y = parent_pos.y + self_offset.y;
                break;
            }

            p.x += parent_spacing.x;
            p.y += parent_spacing.y;

            if (m_parent->m_alignment.has_any(ContainerAlignment::Bottom))
            {
                p.y = parent_size.y - self_size.y;
            }

            if (m_parent->m_alignment.has_any(ContainerAlignment::CenterX) && m_parent->m_layout == ContainerLayout::Vertical)
            {
                p.x += parent_size.x / 2 - self_size.x / 2;
            }
            else if (m_parent->m_alignment.has_any(ContainerAlignment::CenterY) && m_parent->m_layout == ContainerLayout::Horizontal)
            {
                p.y += parent_size.y / 2 - self_size.y / 2;
            }
        }
        else
        {
            p.x = parent_pos.x + self_offset.x;
            p.y = parent_pos.y + self_offset.y;

            if (m_parent->m_alignment.has_any(ContainerAlignment::Right))
            {
                p.x += parent_size.x - self_size.x;
            }
            if (m_parent->m_alignment.has_any(ContainerAlignment::CenterX))
            {
                p.x += parent_size.x / 2 - self_size.x / 2;
            }
            if (m_parent->m_alignment.has_any(ContainerAlignment::CenterY))
            {
                p.y += parent_size.y / 2 - self_size.y / 2;
            }
            if (m_parent->m_alignment.has_any(ContainerAlignment::Bottom))
            {
                p.y = parent_size.y - self_size.y;
            }
        }
    }

    return p;
}

GlobalPoint Widget::get_global_size() const
{
    GlobalPoint p{};

    Point self_size = size();

    switch (m_layout)
    {
    case ContainerLayout::Vertical:
    {
        int32_t max_width = 0;

        for (std::shared_ptr<Widget> child : m_children)
        {
            GlobalPoint p2 = child->get_global_size();
            p.y += p2.y;

            if (p2.x > max_width)
                max_width = p2.x;
        }

        p.x = max_width;
    }
    break;
    case ContainerLayout::Horizontal:
    {
        int32_t max_height = 0;

        for (std::shared_ptr<Widget> child : m_children)
        {
            GlobalPoint p2 = child->get_global_size();
            p.x += p2.x;

            if (p2.y > max_height)
                max_height = p2.y;
        }

        p.y = max_height;
    }
    break;
    case ContainerLayout::Stack:
    {
        int32_t max_width = 0;
        int32_t max_height = 0;

        for (std::shared_ptr<Widget> child : m_children)
        {
            GlobalPoint p2 = child->get_global_size();

            if (p2.x > max_width)
                max_width = p2.x;
            if (p2.y > max_height)
                max_height = p2.y;
        }

        p.x = max_width;
        p.y = max_height;
    }
    break;
    }

    p.x = std::max(p.x, get_width(self_size.x));
    p.y = std::max(p.y, get_height(self_size.y));

    if (m_parent != nullptr)
    {
        Point parent_size = m_parent->size();

        if (m_expand_horizontal)
            p.x = m_parent->get_width(parent_size.x);
        if (m_expand_vertical)
            p.y = m_parent->get_height(parent_size.y);
    }
    else
    {
        if (m_expand_horizontal)
            p.x = get_width(Size::percent(100.0));
        if (m_expand_vertical)
            p.y = get_height(Size::percent(100.0));
    }

    return p;
}

int32_t Widget::get_width(Size size) const
{
    if (size.type == SizeType::Px)
        return size.data.px;

    Extent2D extent = Engine::get().window()->size();
    return int32_t(float(extent.width) * (size.data.percent / 100.0f));
}

int32_t Widget::get_height(Size size) const
{
    if (size.type == SizeType::Px)
        return size.data.px;

    Extent2D extent = Engine::get().window()->size();
    return int32_t(float(extent.height) * (size.data.percent / 100.0f));
}

GlobalPoint Widget::globalize(Point point) const
{
    return GlobalPoint(get_width(point.x), get_height(point.y));
}

size_t Widget::get_child_index(const Widget *widget)
{
    for (size_t i = 0; i < m_children.size(); i++)
    {
        if (m_children[i].get() == widget)
            return i;
    }
    return 0;
}

void Widget::draw_everything(const RenderPass& pass)
{
    if (!m_visible)
        return;

    draw(pass);

    for (size_t i = 0; i < m_children.size(); i++)
        m_children[i]->draw_everything(pass);
}

void Widget::update_everything(float delta)
{
    if (!m_enabled)
        return;

    update(delta);

    for (size_t i = 0; i < m_children.size(); i++)
        m_children[i]->update_everything(delta);
}

Animation& Widget::animate(AnimationType type, std::string_view property)
{
    std::shared_ptr<Animation> animation = std::make_shared<Animation>(std::string(property));
    m_animations[type].push_back(animation);
    return *animation.get();
}

bool Widget::is_mouse_hovering() const
{
    if (Input::is_mouse_grabbed())
        return false;

    const GlobalPoint pos = get_global_pos();
    const GlobalPoint size = get_global_size();

    const glm::i32vec2 mouse_coords = Input::get_mouse_coordinates();
    return mouse_coords.x >= pos.x && mouse_coords.x <= pos.x + size.x && mouse_coords.y >= pos.y && mouse_coords.y <= pos.y + size.y;
}

uint64_t Widget::new_anim_id()
{
    uint64_t id = m_anim_id;
    m_anim_id++;
    return id;
}

void Widget::cancel_animations(AnimationType type, bool reset)
{
    std::set<uint64_t> ids;

    for (size_t i = 0; i < m_runtime_animations.size(); i++)
    {
        const RuntimeAnimation& animation = m_runtime_animations[i];
        if (animation.type == type)
        {
            if (reset)
                set(animation.animation->_property, animation.original_value);
            ids.insert(animation.id);
        }
    }

    for (uint64_t id : ids)
    {
        m_runtime_animations.erase(std::find_if(m_runtime_animations.begin(), m_runtime_animations.end(), [id](const auto& a)
                                                { return a.id == id; }));
    }
}

// ------- ColorRectWidget

void ColorRectWidget::bind_static()
{
    type.add_property("color", &ColorRectWidget::get_color, &ColorRectWidget::set_color);
}

ColorRectWidget::ColorRectWidget()
{
    m_buffer = EXPECT(Buffer::create(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_bg = BindGroup::create(Renderer::get().get_color_rect_shader());
    m_bg->set_param("env", Renderer::get().get_env_2d());
    m_bg->set_param("uniforms", m_buffer);
}

Point ColorRectWidget::size() const
{
    return m_size;
}

void ColorRectWidget::draw(const RenderPass& pass)
{
    // TODO: cache the value, only update when the widget tree is dirty.
    GlobalPoint pos = get_global_pos();
    GlobalPoint size = get_global_size();
    Extent2D window_size = Engine::get().window()->size();

    const float aspect_ratio = float(window_size.width) / float(window_size.height);

    float width = (float(size.x) / float(window_size.width - 1)) * aspect_ratio;
    float height = float(size.y) / float(window_size.height - 1);

    float x = (float(pos.x) / float(window_size.width - 1)) * aspect_ratio + width / 2.0f;
    float y = float(pos.y) / float(window_size.height - 1) + height / 2.0f;

    m_uniforms.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, 0.1)) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(width, height, 0.0));

    m_buffer->update_struct(m_uniforms);
    Renderer::get().draw(pass, Renderer::get().get_square_mesh(), Renderer::get().get_fw_color_rect_mat(), m_bg);
}

void ColorRectWidget::set_color(Color color)
{
    m_uniforms.color = color;
}

// ------- TextureRectWidget

void TextureRectWidget::bind_static()
{
    type.add_property("color", &TextureRectWidget::get_color, &TextureRectWidget::set_color);
}

TextureRectWidget::TextureRectWidget()
{
    m_buffer = EXPECT(Buffer::create(sizeof(Uniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_bg = BindGroup::create(Renderer::get().get_texture_rect_shader());
    m_bg->set_param("env", Renderer::get().get_env_2d());
    m_bg->set_param("uniforms", m_buffer);
}

void TextureRectWidget::set_texture(std::shared_ptr<Texture> texture)
{
    m_bg->set_param("image", texture);
    m_texture = texture;
}

void TextureRectWidget::draw(const RenderPass& pass)
{
    // TODO: cache the value, only update when the widget tree is dirty.
    GlobalPoint pos = get_global_pos();
    GlobalPoint size = get_global_size();
    Extent2D window_size = Engine::get().window()->size();

    const float aspect_ratio = float(window_size.width) / float(window_size.height);

    float width = (float(size.x) / float(window_size.width - 1)) * aspect_ratio;
    float height = float(size.y) / float(window_size.height - 1);

    float x = (float(pos.x) / float(window_size.width - 1)) * aspect_ratio + width / 2.0f;
    float y = float(pos.y) / float(window_size.height - 1) + height / 2.0f;

    m_uniforms.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, 0.1)) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(width, height, 0.0));

    m_buffer->update_struct(m_uniforms);
    Renderer::get().draw(pass, Renderer::get().get_square_mesh(), Renderer::get().get_fw_texture_rect_mat(), m_bg);
}

// ------- LabelWidget

void LabelWidget::bind_static()
{
    type.add_property("color", &LabelWidget::get_color, &LabelWidget::set_color);
}

LabelWidget::LabelWidget(std::shared_ptr<Font> font)
    : m_text(font)
{
}

Point LabelWidget::size() const
{
    return Point(Size::px(m_text.get_width()), Size::px(m_text.get_height()));
}

void LabelWidget::draw(const RenderPass& pass)
{
    // TODO: cache the value, only update when the widget tree is dirty.
    GlobalPoint pos = get_global_pos();
    GlobalPoint size = get_global_size();
    Extent2D window_size = Engine::get().window()->size();

    const float aspect_ratio = float(window_size.width) / float(window_size.height);

    float width = (float(size.x) / float(window_size.width - 1)) * aspect_ratio;
    float height = float(size.y) / float(window_size.height - 1);

    float x = (float(pos.x) / float(window_size.width - 1)) * aspect_ratio + width / 2.0f;
    float y = float(pos.y) / float(window_size.height - 1) + height / 2.0f;

    m_text.set_position(glm::vec3(x - width / 4.0f, y, 0.1));
    m_text.set_scale(glm::vec2(1.0, 1.0));

    m_text.draw(pass);
}

void LabelWidget::set_color(Color color)
{
    m_color = color;
    m_text.set_color(color.to_vec());
}

void LabelWidget::set_text(std::string_view text)
{
    m_text.set(text);
}
