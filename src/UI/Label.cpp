#include "UI/Label.hpp"
#include "Font.hpp"

Label::Label(Ref<Font> font)
    : m_text(10, font)
{
    m_text.set_position(glm::vec3(0, 0, 0));
    m_text.set_color(glm::vec4(1.0, 0.0, 0.0, 1.0));
    m_text.set_scale(glm::vec2(0.3));
}

void Label::set_text(String text)
{
    m_text.set(text);
}

void Label::update(float d)
{
    (void)d;
    m_text.set_position(glm::vec3(m_position, 0.0));
}

void Label::process_event(Event& event)
{
    (void)event;
}

void Label::draw(const RenderPassNode& node)
{
    m_text.draw(node);
}
