#pragma once

#include "UI/UI.hpp"

class Container : public UI
{
    CLASS(Container, UI);

public:
    virtual ~Container() {}

    void add_child(std::shared_ptr<UI> ui)
    {
        m_children.push_back(ui);
    }

    std::span<std::shared_ptr<UI>> get_children() { return std::span<std::shared_ptr<UI>>(m_children); }

    virtual void update(float d) override
    {
        for (std::shared_ptr<UI> child : m_children)
            child->update(d);
    }

    virtual void process_event(Event& event) override
    {
        for (const std::shared_ptr<UI>& child : m_children)
            child->process_event(event);
    }

    virtual void draw(const RenderPass& pass) override
    {
        for (const std::shared_ptr<UI>& child : m_children)
            child->draw(pass);
    }

private:
    std::vector<std::shared_ptr<UI>> m_children;
};
