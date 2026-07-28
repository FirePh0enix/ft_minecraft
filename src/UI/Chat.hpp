#pragma once

#include "Core/Class.hpp"
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
    virtual void draw(const RenderPass& pass) override;
    virtual void process_event(Event& event) override;

private:
    std::string m_buffer;
    std::shared_ptr<ColorRect> m_background;
    std::shared_ptr<Label> m_label;
    Chat *m_chat;
};

class Chat : public Container
{
    CLASS(Chat, Container);

public:
    Chat();

    Result<void> add_line(const std::string& line);

private:
    std::vector<std::string> m_lines;
    std::vector<std::shared_ptr<Label>> m_labels;
};
