#pragma once

#include "Signal.hpp"
#include "UI/Widget.hpp"

class TextInput : public Widget
{
public:
    static void bind_static();

    TextInput(std::shared_ptr<Font> font);

    virtual void draw(const RenderPass& pass) override;
    virtual void process_event(Event& event) override;

    Signal<TextInput&, std::string_view>& done_callback() { return m_done_callback; }
    const Signal<TextInput&, std::string_view>& done_callback() const { return m_done_callback; }

    void clear() { m_text.set(""); }
    void set_text(std::string_view string) { m_text.set(string); }

private:
    Text m_text;
    Color m_color;

    Signal<TextInput&, std::string_view> m_done_callback;
};
