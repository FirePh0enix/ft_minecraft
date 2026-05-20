#pragma once

#include "Core/Containers/LocalVector.hpp"
#include "UI/UI.hpp"

class Container : public UI
{
    CLASS(Container, UI);

public:
    virtual ~Container() {}

    Result<void> add_child(Ref<UI> ui)
    {
        TRY(m_children.append(ui));
        return Result<void>();
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

    virtual void draw(const RenderPassNode& node) override
    {
        for (Ref<UI> child : m_children)
            child->draw(node);
    }

private:
    LocalVector<Ref<UI>> m_children;
};
