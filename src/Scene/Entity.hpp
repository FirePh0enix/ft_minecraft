#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Scene/Components/Component.hpp"

enum class EntityId : uint32_t
{
};

class Entity : public Object
{
    CLASS(Entity, Object);

public:
    Entity();
    virtual ~Entity() {}

    inline EntityId get_id() const
    {
        return m_id;
    }

    inline void set_id(EntityId id)
    {
        m_id = id;
    }

    template <typename T>
    Ref<T> get_component() const
    {
        for (const auto& comp : m_components)
        {
            if (comp->is<T>())
                return comp.cast_to<T>();
        }
        return nullptr;
    }

    template <typename T>
    Ref<T> get_component(size_t index) const
    {
        return m_components[index].cast_to<T>();
    }

    template <typename T>
    void add_component(const Ref<T>& comp)
    {
        m_components.push_back(comp.template cast_to<Component>());
    }

    void start()
    {
        for (auto& comp : m_components)
            comp->start();
    }

    void tick()
    {
        for (auto& comp : m_components)
            comp->tick();
    }

private:
    EntityId m_id;
    std::vector<Ref<Component>> m_components;
};
