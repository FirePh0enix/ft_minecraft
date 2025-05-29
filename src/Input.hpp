#pragma once

#include "Window.hpp"

#include <print>

enum class Action : uint8_t
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,

    Max,
};

struct Status
{
    float value = 0.0;
};

class Input
{
public:
    Input()
    {
    }

    bool is_action_pressed(Action action) const
    {
        return m_actions[(size_t)action].value > 0.0;
    }

    float get_action_value(Action action) const
    {
        return m_actions[(size_t)action].value;
    }

    void set_action_value(Action action, float value)
    {
        m_actions[(size_t)action].value = value;
    }

    glm::vec3 get_movement_vector() const
    {
        return glm::vec3(get_action_value(Action::Right) - get_action_value(Action::Left), get_action_value(Action::Up) - get_action_value(Action::Down), get_action_value(Action::Forward) - get_action_value(Action::Backward));
    }

    void set_mouse_grabbed(const Window& window, bool value)
    {
        m_mouse_grabbed = value;
        SDL_SetWindowRelativeMouseMode(window.get_window_ptr(), value);
    }

    bool is_mouse_grabbed() const
    {
        return m_mouse_grabbed;
    }

    void process_event(Window& window, SDL_Event event)
    {
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event.key.key)
            {
            case SDLK_ESCAPE:
                set_mouse_grabbed(window, false);
                break;

            case SDLK_W:
                set_action_value(Action::Forward, 1.0);
                break;
            case SDLK_S:
                set_action_value(Action::Backward, 1.0);
                break;
            case SDLK_A:
                set_action_value(Action::Left, 1.0);
                break;
            case SDLK_D:
                set_action_value(Action::Right, 1.0);
                break;
            case SDLK_SPACE:
                set_action_value(Action::Up, 1.0);
                break;
            case SDLK_LCTRL:
                set_action_value(Action::Down, 1.0);
                break;

            default:
                break;
            }
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            switch (event.key.key)
            {
            case SDLK_W:
                set_action_value(Action::Forward, 0.0);
                break;
            case SDLK_S:
                set_action_value(Action::Backward, 0.0);
                break;
            case SDLK_A:
                set_action_value(Action::Left, 0.0);
                break;
            case SDLK_D:
                set_action_value(Action::Right, 0.0);
                break;
            case SDLK_SPACE:
                set_action_value(Action::Up, 0.0);
                break;
            case SDLK_LCTRL:
                set_action_value(Action::Down, 0.0);
                break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            set_mouse_grabbed(window, true);
            break;
        }
        }
    }

    static Input& get()
    {
        return input;
    }

private:
    static Input input;

    std::array<Status, (size_t)Action::Max> m_actions;
    bool m_mouse_grabbed = false;
};
