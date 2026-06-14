#pragma once

#include "Core/Containers/LocalVector.hpp"
#include "UI/UI.hpp"

class Container : public UI
{
    CLASS(Container, UI);

public:
    virtual ~Container() {}

    void add_child(Ref<UI> ui)
    {
        m_children.append(ui);
    }

    View<Ref<UI>> get_children() const { return m_children; }

    virtual void update(float d) override
    {
        for (Ref<UI> child : m_children)
            child->update(d);
    }

    virtual void process_event(Event& event) override
    {
        for (Ref<UI> child : m_children)
            child->process_event(event);
    }

    virtual void draw(const RenderPass& pass) override
    {
        for (Ref<UI> child : m_children)
            child->draw(pass);
    }

private:
    LocalVector<Ref<UI>> m_children;
};
