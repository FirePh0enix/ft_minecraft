#include "UI/Chat.hpp"
#include "Core/Ref.hpp"
#include "Engine.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "UI/ColorRect.hpp"
#include "UI/Label.hpp"

ChatInput::ChatInput(Chat *chat)
    : m_chat(chat)
{
    m_scale = glm::vec2(1.5, 0.08);
    m_position = glm::vec2(-0.95, -0.92);

    m_background = newref<ColorRect>();
    m_background->set_scale(m_scale);
    m_background->set_color(Color(0.01, 1.0));
    m_background->set_position(m_position);

    constexpr float scale = 0.05f;
    m_label = newref<Label>(Engine::get().get_font());
    m_label->set_scale(glm::vec2(scale));
    m_label->set_position(glm::vec2(-1.6, -0.925));
}

void ChatInput::update(float d)
{
    if (is_mouse_hovering())
    {
        m_background->set_color(Color(0.05, 1.0));
    }
    else
    {
        m_background->set_color(Color(0.01, 1.0));
    }

    m_background->update(d);
    m_label->update(d);
}

void ChatInput::process_event(Event& event)
{
    if (event.event.type == SDL_EVENT_TEXT_INPUT)
    {
        const char *s = event.event.text.text;
        m_buffer.append(s);
        m_label->set_text(m_buffer);
    }
    else if (event.event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.event.key.key == SDLK_BACKSPACE && m_buffer.size() > 0)
        {
            m_buffer.resize(m_buffer.size() - 1);
            m_label->set_text(m_buffer);
        }
        if (event.event.key.key == SDLK_RETURN && m_buffer.size() > 0)
        {
            EXPECT(m_chat->add_line(m_buffer));
            m_buffer.resize(0);
            m_label->set_text(m_buffer);
        }
    }
}

void ChatInput::draw(WGPURenderPassEncoder encoder)
{
    m_background->draw(encoder);
    m_label->draw(encoder);
}

Chat::Chat()
{
    m_scale = glm::vec2(1.5, 1.0);

    Ref<ColorRect> background = newref<ColorRect>();
    background->set_scale(m_scale);
    background->set_color(Color(0.02, 0.6));
    background->set_position(glm::vec2(-0.95, -0.45));
    add_child(background);

    Ref<ChatInput> input = newref<ChatInput>(this);
    add_child(input);
}

Result<void> Chat::add_line(const String& line)
{
    m_lines.append(line);

    Ref<Font> font = Engine::get().get_font();
    constexpr float scale = 0.05f;

    for (auto& label : m_labels)
    {
        label->set_position(label->get_position() + glm::vec2(0, scale));
    }

    Ref<Label> label = newref<Label>(font);
    label->set_text(line);
    label->set_scale(glm::vec2(scale));
    label->set_position(glm::vec2(-1.6, -0.80));

    m_labels.append(label);
    add_child(label);

    return Result<void>();
}
