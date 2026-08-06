#pragma once

#include "../Font.hpp"
#include "UI/UI.hpp"

class Label : public UI
{
    CLASS(Label, UI);

public:
    Label(std::shared_ptr<Font> font);

    void set_text(const std::string& text);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(const RenderPass& pass) override;

private:
    Text m_text;
};
