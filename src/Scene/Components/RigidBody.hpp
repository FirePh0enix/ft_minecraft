#pragma once

#include "Physics/PhysicsBody.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Components/Transformed3D.hpp"

class RigidBody : public Component
{
    CLASS(RigidBody, Component);

public:
    static void bind_methods()
    {
        ClassRegistry::get().register_property<RigidBody>("disabled", PrimitiveType::Bool, [](RigidBody *self)
                                                          { return Variant(self->is_disabled()); }, [](RigidBody *self, Variant value)
                                                          { self->set_disabled(value); });
    }

    RigidBody(Collider *collider);

    virtual ~RigidBody();

    virtual void start() override;
    // virtual void tick(double delta) override;

    bool is_disabled() const { return m_disabled; }
    void set_disabled(bool disabled) { m_disabled = disabled; }

    PhysicsBody *get_body() const { return m_body; }

private:
    Ref<Transformed3D> m_transform;
    PhysicsBody *m_body = nullptr;

    bool m_disabled = false;
};
