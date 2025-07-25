#pragma once

#include "Window.hpp"

enum class Action : uint8_t
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,

    Attack,
    Place,

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

    inline glm::vec2 get_mouse_relative()
    {
        return m_mouse_relative;
    }

    void post_events()
    {
        m_mouse_relative = glm::vec2();
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
            if (!is_mouse_grabbed())
            {
                set_mouse_grabbed(window, true);
                break;
            }

            if (event.button.button == SDL_BUTTON_LEFT)
            {
                set_action_value(Action::Attack, 1.0);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                set_action_value(Action::Place, 1.0);
            }
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                set_action_value(Action::Attack, 0.0);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                set_action_value(Action::Place, 0.0);
            }
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            m_mouse_relative.x = event.motion.xrel;
            m_mouse_relative.y = event.motion.yrel;
        }
        break;
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
    glm::vec2 m_mouse_relative = glm::vec2(0.0);
};
