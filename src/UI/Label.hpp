#pragma once

#include "Font.hpp"
#include "UI/UI.hpp"

class Label : public UI
{
    CLASS(Label, UI);

public:
    Label(Ref<Font> font);

    void set_text(String text);

    virtual void update(float d) override;
    virtual void process_event(Event& event) override;
    virtual void draw(WGPURenderPassEncoder encoder) override;

private:
    Text m_text;
};
