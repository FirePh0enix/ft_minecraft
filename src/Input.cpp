#include "Input.hpp"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"

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

    Input::add_action("jump");
    Input::add_action_mapping("jump", ActionMapping(ActionMappingKind::Key, SDLK_SPACE));

    Input::add_action("down");
    Input::add_action_mapping("down", ActionMapping(ActionMappingKind::Key, SDLK_LCTRL));

    Input::add_action("attack");
    Input::add_action_mapping("attack", ActionMapping(ActionMappingKind::MouseButton, 1));

    Input::add_action("interact");
    Input::add_action_mapping("interact", ActionMapping(ActionMappingKind::MouseButton, 3));

    Input::add_action("escape");
    Input::add_action_mapping("escape", ActionMapping(ActionMappingKind::Key, SDLK_ESCAPE));

    Input::add_action("open_inventory");
    Input::add_action_mapping("open_inventory", ActionMapping(ActionMappingKind::Key, SDLK_E));

    Input::add_action("toggle_chat");
    Input::add_action_mapping("toggle_chat", ActionMapping(ActionMappingKind::Key, SDLK_T));

    Input::add_action("1");
    Input::add_action_mapping("1", ActionMapping(ActionMappingKind::Key, SDLK_1));

    Input::add_action("2");
    Input::add_action_mapping("2", ActionMapping(ActionMappingKind::Key, SDLK_2));

    Input::add_action("3");
    Input::add_action_mapping("3", ActionMapping(ActionMappingKind::Key, SDLK_3));

    Input::add_action("4");
    Input::add_action_mapping("4", ActionMapping(ActionMappingKind::Key, SDLK_4));

    Input::add_action("5");
    Input::add_action_mapping("5", ActionMapping(ActionMappingKind::Key, SDLK_5));

    Input::add_action("6");
    Input::add_action_mapping("6", ActionMapping(ActionMappingKind::Key, SDLK_6));

    Input::add_action("7");
    Input::add_action_mapping("7", ActionMapping(ActionMappingKind::Key, SDLK_7));

    Input::add_action("8");
    Input::add_action_mapping("8", ActionMapping(ActionMappingKind::Key, SDLK_8));

    Input::add_action("9");
    Input::add_action_mapping("9", ActionMapping(ActionMappingKind::Key, SDLK_9));

    Input::add_action("toolbar_wheel");
    Input::add_action_mapping("toolbar_wheel", ActionMapping(ActionMappingKind::Wheel, 0));

    // UI
    Input::add_action("ui_click");
    Input::add_action_mapping("ui_click", ActionMapping(ActionMappingKind::MouseButton, 1));
}

bool Input::is_action_pressed(const StringView& action)
{
    Option<Status> status_opt = s_actions.get(action);
    if (!status_opt.has_value())
        return false;
    return status_opt.get().value > 0.0;
}

bool Input::is_action_just_pressed(const StringView& action)
{
    Option<Status> status_opt = s_actions.get(action);
    if (!status_opt.has_value())
        return false;
    return status_opt.get().value > 0.0 && !status_opt.get().repeat;
}

float Input::get_action_value(const StringView& action)
{
    Option<Status> status_opt = s_actions.get(action);
    if (!status_opt.has_value())
        return false;
    return status_opt.get().value;
}

void Input::set_action_value(const StringView& action, float value)
{
    Status *status = s_actions.get_or_put(action, {});
    status->value = value;

    if (value == 0.0)
        status->repeat = false;
}

glm::vec2 Input::get_vector(const StringView& x_negative, const StringView& x_positive, const StringView& y_negative, const StringView& y_positive)
{
    return glm::vec2(get_action_value(x_positive) - get_action_value(x_negative), get_action_value(y_positive) - get_action_value(y_negative));
}

float Input::get_axis(const StringView& negative, const StringView& positive)
{
    return get_action_value(positive) - get_action_value(negative);
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

glm::vec2 Input::get_mouse_absolute()
{
    float x, y;
    SDL_GetMouseState(&x, &y);

    int w, h;
    SDL_GetWindowSize(s_window, &w, &h);

    return ((glm::vec2(x, y) / glm::vec2(w - 1, h - 1)) * 2.0f - 1.0f) * glm::vec2(float(w) / float(h), -1.0f);
}

void Input::post_events()
{
    s_mouse_relative = glm::vec2();

    for (auto& [_, key, status] : s_actions)
    {
        if (status.value > 0)
            status.repeat = true;

        if (s_mappings.get(key).get().get_unchecked(0).kind == ActionMappingKind::Wheel)
        {
            status.repeat = false;
            status.value = 0.0;
        }
    }
}

void Input::process_event(SDL_Event event)
{
    for (const auto& mappings : s_mappings)
    {
        for (const auto& mapping : mappings.value)
        {
            if (event.type == SDL_EVENT_KEY_DOWN && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(mappings.key, 1.0);
            }
            else if (event.type == SDL_EVENT_KEY_UP && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(mappings.key, 0.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(mappings.key, 1.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(mappings.key, 0.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL && mapping.kind == ActionMappingKind::Wheel)
            {
                if (event.wheel.y < 0)
                    set_action_value(mappings.key, 1.0);
                else
                    set_action_value(mappings.key, -1.0);
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        s_mouse_relative.x = event.motion.xrel;
        s_mouse_relative.y = event.motion.yrel;
    }
}

void Input::add_action(const StringView& name)
{
    s_mappings.put(name, {});
}

void Input::add_action_mapping(const StringView& name, ActionMapping mapping)
{
    Vector<ActionMapping>& mappings = *s_mappings.get_ptr(name).get();
    mappings.append(mapping);
}
