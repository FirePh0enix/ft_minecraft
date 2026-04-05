#include "Input.hpp"
#include "SDL3/SDL_mouse.h"

void Input::init(const Window& window)
{
#ifndef __platform_web
    s_window = window.get_window_ptr();
#endif
}

void Input::load_config()
{
    Input::add_action("forward");
    Input::add_action_mapping("forward", ActionMapping(ActionMappingKind::Key, SDLK_W));

    Input::add_action("backward");
    Input::add_action_mapping("backward", ActionMapping(ActionMappingKind::Key, SDLK_S));

    Input::add_action("left");
    Input::add_action_mapping("left", ActionMapping(ActionMappingKind::Key, SDLK_A));

    Input::add_action("right");
    Input::add_action_mapping("right", ActionMapping(ActionMappingKind::Key, SDLK_D));

    Input::add_action("up");
    Input::add_action_mapping("up", ActionMapping(ActionMappingKind::Key, SDLK_SPACE));

    Input::add_action("down");
    Input::add_action_mapping("down", ActionMapping(ActionMappingKind::Key, SDLK_LCTRL));

    Input::add_action("attack");
    Input::add_action_mapping("attack", ActionMapping(ActionMappingKind::MouseButton, 1));

    Input::add_action("interact");
    Input::add_action_mapping("interact", ActionMapping(ActionMappingKind::MouseButton, 3));

    Input::add_action("escape");
    Input::add_action_mapping("escape", ActionMapping(ActionMappingKind::Key, SDLK_ESCAPE));
}

bool Input::is_action_pressed(const String& action)
{
    return s_actions[action].value > 0.0;
}

float Input::get_action_value(const String& action)
{
    return s_actions[action].value;
}

void Input::set_action_value(const String& action, float value)
{
    s_actions[action].value = value;
}

glm::vec2 Input::get_vector(const String& x_negative, const String& x_positive, const String& y_negative, const String& y_positive)
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

void Input::add_action(const String& name)
{
    s_mappings[name] = {};
}

void Input::add_action_mapping(const String& name, ActionMapping mapping)
{
    Vector<ActionMapping>& mappings = s_mappings[name];
    (void)mappings.append(mapping);
}
