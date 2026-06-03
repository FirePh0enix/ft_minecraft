#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/String.hpp"
#include "UI/ColorRect.hpp"
#include "UI/Container.hpp"
#include "UI/Label.hpp"

class Chat;

class ChatInput : public UI
{
    CLASS(ChatInput, UI);

public:
    ChatInput(Chat *chat);

    virtual void update(float d) override;
    virtual void draw(const RenderPassNode& node) override;
    virtual void process_event(Event& event) override;

private:
    String m_buffer;
    Ref<ColorRect> m_background;
    Ref<Label> m_label;
    Chat *m_chat;
};

class Chat : public Container
{
    CLASS(Chat, Container);

public:
    Chat();

    Result<void> add_line(const String& line);

private:
    LocalVector<String> m_lines;
    LocalVector<Ref<Label>> m_labels;
};
