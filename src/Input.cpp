#include "Input.hpp"

void Input::init(const Window& window)
{
#ifndef __platform_web
    s_window = window.get_window_ptr();
#endif
}

bool Input::is_action_pressed(const std::string& action)
{
    return s_actions[action].value > 0.0;
}

float Input::get_action_value(const std::string& action)
{
    return s_actions[action].value;
}

void Input::set_action_value(const std::string& action, float value)
{
    s_actions[action].value = value;
}

glm::vec2 Input::get_vector(const std::string& x_negative, const std::string& x_positive, const std::string& y_negative, const std::string& y_positive)
{
    return glm::vec2(get_action_value(x_positive) - get_action_value(x_negative), get_action_value(y_positive) - get_action_value(y_negative));
}

void Input::set_mouse_grabbed(bool value)
{
    s_mouse_grabbed = value;
#ifndef __platform_web
    SDL_SetWindowRelativeMouseMode(s_window, value);
#endif
}

bool Input::is_mouse_grabbed()
{
    return s_mouse_grabbed;
}

glm::vec2 Input::get_mouse_relative()
{
    return s_mouse_relative;
}

void Input::post_events()
{
    s_mouse_relative = glm::vec2();
}

void Input::process_event(SDL_Event event)
{
    for (const auto& mappings : s_mappings)
    {
        for (const auto& mapping : mappings.second)
        {
            if (event.type == SDL_EVENT_KEY_DOWN && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(mappings.first, 1.0);
            }
            else if (event.type == SDL_EVENT_KEY_UP && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(mappings.first, 0.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(mappings.first, 1.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(mappings.first, 0.0);
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        s_mouse_relative.x = event.motion.xrel;
        s_mouse_relative.y = event.motion.yrel;
    }
}

void Input::add_action(const std::string& name)
{
    s_mappings[name] = {};
}

void Input::add_action_mapping(const std::string& name, ActionMapping mapping)
{
    std::vector<ActionMapping>& mappings = s_mappings[name];
    mappings.push_back(mapping);
}
