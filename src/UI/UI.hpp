#pragma once

#include "Core/Class.hpp"
#include "Event.hpp"
#include "Input.hpp"
#include "Render/Renderer.hpp"

struct Rect
{
    float x;
    float y;
    float width;
    float height;
};

class UI : public Object
{
    CLASS(UI, Object);

public:
    virtual ~UI() {}

    virtual void update(float d) = 0;
    virtual void draw(const RenderPass& pass) = 0;

    virtual void process_event(Event& event)
    {
        (void)event;
    }

    void set_position(glm::vec2 position) { m_position = position; }
    glm::vec2 get_position() const { return m_position; }

    void set_scale(glm::vec2 scale) { m_scale = scale; }
    glm::vec2 get_scale() const { return m_scale; }

    void set_index(uint32_t index) { m_index = index; }
    uint32_t get_index() const { return m_index; }

    Rect get_rect()
    {
        Rect r{};
        r.x = m_position.x - m_scale.x / 2.0f;
        r.y = m_position.y - m_scale.y / 2.0f;
        r.width = m_scale.x;
        r.height = m_scale.y;
        return r;
    }

    bool is_mouse_hovering()
    {
        if (Input::is_mouse_grabbed())
            return false;
        const Rect rect = get_rect();
        const glm::vec2 mouse_coords = Input::get_mouse_absolute();
        return mouse_coords.x >= rect.x && mouse_coords.x <= rect.x + rect.width && mouse_coords.y >= rect.y && mouse_coords.y <= rect.y + rect.height;
    }

protected:
    glm::vec2 m_position = glm::vec2();
    float m_rotation = 0;
    glm::vec2 m_scale = glm::vec2(1.0);
    uint32_t m_index = 0;

    glm::mat4 matrix() const
    {
        return glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_position, float(m_index))) * glm::rotate(glm::identity<glm::mat4>(), m_rotation, glm::vec3(0, 0, 1)) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(m_scale, 1.0));
    }
};
