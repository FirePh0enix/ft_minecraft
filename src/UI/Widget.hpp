#pragma once

#include "Color.hpp"
#include "Core/Flags.hpp"
#include "Event.hpp"
#include "Font.hpp"
#include "Render/Renderer.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

enum class SizeType
{
    Px,
    Percent,
};

struct Size
{
    SizeType type = SizeType::Px;
    union
    {
        int32_t px = 0;
        float percent;
    } data;

    static Size px(int32_t value)
    {
        return Size{.type = SizeType::Px, .data{.px = value}};
    }

    static Size percent(float value)
    {
        return Size{.type = SizeType::Percent, .data{.percent = value}};
    }
};

struct Point
{
    Size x;
    Size y;

    Point()
    {
    }

    Point(Size scalar)
        : x(scalar), y(scalar)
    {
    }

    Point(Size x, Size y)
        : x(x), y(y)
    {
    }
};

struct WidgetRect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct GlobalPoint
{
    int32_t x;
    int32_t y;
};

enum class ContainerLayout
{
    Vertical,
    Horizontal,
    Stack,
};

enum class ContainerAlignment
{
    Left = 1 << 0,
    Right = 1 << 1,
    Bottom = 1 << 2,
    Top = 1 << 3,
    CenterX = 1 << 4,
    CenterY = 1 << 5,

    Center = CenterX | CenterY,
};
using ContainerAlignmentFlags = Flags<ContainerAlignment>;
DEFINE_FLAG_TRAITS(ContainerAlignment);

enum class AnimationType
{
    HoverIn,
    HoverOut,
};

enum class TransitionType
{
    /// Lerp between the original value and the target.
    Lerp,
    /// Update the target value after the timer has ended.
    Set,
};

struct Animation
{
    std::string _property;
    Variant _value;
    float _time = 0.0f;
    TransitionType _transition = TransitionType::Lerp;

    Animation(std::string property)
        : _property(property)
    {
    }

    Animation& to(Variant value)
    {
        _value = value;
        return *this;
    }

    Animation& time(float time)
    {
        _time = time;
        return *this;
    }

    Animation& transition(TransitionType transition)
    {
        _transition = transition;
        return *this;
    }
};

class Widget : public Object
{
    CLASS(Widget, Object);

public:
    static void bind_static();

    virtual Point size() const { return m_size; }

    virtual void update(float delta);

    virtual void draw(const RenderPass& pass) { (void)pass; };
    virtual void process_event(Event& event) { (void)event; };

    Widget *get_parent() const { return m_parent; }
    void set_parent(Widget *parent) { m_parent = parent; }
    bool has_parent() const { return m_parent != nullptr; }

    void add_child(std::shared_ptr<Widget> widget);
    std::span<const std::shared_ptr<Widget>> get_children() const { return m_children; }

    Point get_offset() const { return m_offset; }
    void set_offset(Point offset) { m_offset = offset; }

    Point get_size() const { return m_size; }
    void set_size(Point size) { m_size = size; }

    Point get_spacing() const { return m_spacing; }
    void set_spacing(Point spacing) { m_spacing = spacing; }

    float get_rotation() const { return m_rotation; }
    void set_rotation(float rotation) { m_rotation = rotation; }

    uint32_t get_index() const { return m_index; }
    void set_index(uint32_t index) { m_index = index; }

    ContainerLayout get_layout() const { return m_layout; }
    void set_layout(ContainerLayout layout) { m_layout = layout; }

    ContainerAlignment get_alignment() const { return m_alignment; }
    void set_alignment(ContainerAlignmentFlags alignment) { m_alignment = alignment; }

    void set_expand_vertical(bool b) { m_expand_vertical = b; }
    bool is_expanding_vertical() const { return m_expand_vertical; }

    void set_expand_horizontal(bool b) { m_expand_horizontal = b; }
    bool is_expanding_horizontal() const { return m_expand_horizontal; }

    GlobalPoint get_global_pos() const;
    GlobalPoint get_global_size() const;

    void draw_everything(const RenderPass& pass);
    void update_everything(float delta);

    Animation& animate(AnimationType type, std::string_view property);

    bool is_mouse_hovering() const;

    bool is_visible() const { return m_visible; }
    void set_visible(bool visible) { m_visible = visible; }

    bool is_enabled() const { return m_enabled; }
    void set_enable(bool enable) { m_enabled = enable; }

protected:
    struct RuntimeAnimation
    {
        AnimationType type = AnimationType::HoverIn;
        std::shared_ptr<Animation> animation;
        float timer = 0.0f;
        Variant original_value;
        uint64_t id;
    };

    Widget *m_parent = nullptr;
    std::vector<std::shared_ptr<Widget>> m_children;
    Point m_offset;
    Point m_size;
    Point m_spacing;
    float m_rotation = 0.0;
    uint32_t m_index = 0;
    ContainerLayout m_layout = ContainerLayout::Horizontal;
    ContainerAlignmentFlags m_alignment = ContainerAlignment::Left;
    bool m_expand_vertical = false;
    bool m_expand_horizontal = false;
    bool m_visible = true;
    bool m_enabled = true;

    std::map<AnimationType, std::vector<std::shared_ptr<Animation>>> m_animations;

    // Some states
    std::vector<RuntimeAnimation> m_runtime_animations;
    uint64_t m_anim_id = 0;
    bool m_was_hovering = false;

    int32_t get_width(Size size) const;
    int32_t get_height(Size size) const;
    GlobalPoint globalize(Point point) const;

    size_t get_child_index(const Widget *widget);

    uint64_t new_anim_id();
    void cancel_animations(AnimationType type, bool reset);
};

class ColorRectWidget : public Widget
{
    CLASS(ColorRectWidget, Widget);

public:
    static void bind_static();

    ColorRectWidget();

    virtual Point size() const override;
    virtual void draw(const RenderPass& pass) override;

    void set_color(Color color);
    Color get_color() const { return m_uniforms.color; }

    struct GPU_ATTRIBUTE Uniforms
    {
        glm::mat4 model_matrix = glm::mat4();
        Color color;
    };

protected:
    std::shared_ptr<BindGroup> m_bg;
    std::shared_ptr<Buffer> m_buffer;
    Uniforms m_uniforms;
};

class TextureRectWidget : public Widget
{
    CLASS(TextureRectWidget, Widget);

public:
    static void bind_static();

    TextureRectWidget();

    virtual void draw(const RenderPass& pass) override;

    void set_texture(std::shared_ptr<Texture> texture);

    void set_color(Color color) { m_uniforms.color = color; }
    Color get_color() const { return m_uniforms.color; }

    struct GPU_ATTRIBUTE Uniforms
    {
        glm::mat4 model_matrix = glm::mat4();
        Color color = Colors::white;
    };

private:
    std::shared_ptr<Texture> m_texture;

    std::shared_ptr<BindGroup> m_bg;
    std::shared_ptr<Buffer> m_buffer;
    Uniforms m_uniforms;
};

class LabelWidget : public Widget
{
    CLASS(LabelWidget, Widget);

public:
    static void bind_static();

    LabelWidget(std::shared_ptr<Font> font);

    virtual Point size() const override;
    virtual void draw(const RenderPass& pass) override;

    void set_color(Color color);
    Color get_color() const { return m_color; }

    void set_text(std::string_view text);

private:
    Text m_text;
    Color m_color;
};
