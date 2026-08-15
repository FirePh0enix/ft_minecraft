#include "UI/TextInput.hpp"

#include "Engine.hpp"

void TextInput::bind_static()
{
}

TextInput::TextInput(std::shared_ptr<Font> font)
    : m_text(font)
{
}

void TextInput::process_event(Event& event)
{
    // if (event.handled)
    //     return;

    if (event.event.type == SDL_EVENT_TEXT_INPUT)
    {
        event.handle();

        std::string s(m_text.get_text());
        s += event.event.text.text;

        m_text.set(s);
    }
    else if (event.event.type == SDL_EVENT_KEY_DOWN && event.event.key.key == SDLK_BACKSPACE)
    {
        event.handle();

        if (m_text.get_text().size() > 0)
        {
            std::string s(m_text.get_text());
            s = s.substr(0, s.size() - 1);
            m_text.set(s);
        }
    }
    else if (event.event.type == SDL_EVENT_KEY_DOWN && event.event.key.key == SDLK_RETURN)
    {
        m_done_callback.emit(*this, m_text.get_text());
    }
}

void TextInput::draw(const RenderPass& pass)
{
    GlobalPoint pos = get_global_pos();
    GlobalPoint size = get_global_size();
    Extent2D window_size = Engine::get().window()->size();

    const float aspect_ratio = float(window_size.width) / float(window_size.height);

    float width = (float(size.x) / float(window_size.width - 1)) * aspect_ratio;
    float height = float(size.y) / float(window_size.height - 1);

    float x = (float(pos.x) / float(window_size.width - 1)) * aspect_ratio + width / 2.0f;
    float y = float(pos.y) / float(window_size.height - 1) + height / 2.0f;

    m_text.set_position(glm::vec3(x - width / 2.0f, y, 0.1));
    m_text.set_scale(glm::vec2(1.0, 1.0));

    m_text.draw(pass);
}
