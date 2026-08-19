#include "Input.hpp"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_video.h"

void Input::init(const Window& window)
{
    s_window = window.get_window_ptr();
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

    Input::add_action("middle_click");
    Input::add_action_mapping("middle_click", ActionMapping(ActionMappingKind::MouseButton, 2));

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

    Input::add_action("toggle_debug_menu");
    Input::add_action_mapping("toggle_debug_menu", ActionMapping(ActionMappingKind::Key, SDLK_GRAVE));

    // UI
    Input::add_action("ui_click");
    Input::add_action_mapping("ui_click", ActionMapping(ActionMappingKind::MouseButton, 1));

    Input::add_action("ui_rclick");
    Input::add_action_mapping("ui_rclick", ActionMapping(ActionMappingKind::MouseButton, 3));
}

bool Input::is_action_pressed(std::string_view action)
{
    auto status_opt = s_actions.find(action);
    if (status_opt == s_actions.end())
        return false;
    return status_opt->second.value > 0.0;
}

bool Input::is_action_just_pressed(std::string_view action)
{
    auto status_opt = s_actions.find(action);
    if (status_opt == s_actions.end())
        return false;
    return status_opt->second.value > 0.0 && !status_opt->second.repeat;
}

bool Input::is_action_just_released(const std::string_view& action)
{
    auto status_opt = s_actions.find(action);
    if (status_opt == s_actions.end())
        return false;

    return status_opt->second.released;
}

float Input::get_action_value(std::string_view action)
{
    auto status_opt = s_actions.find(action);
    if (status_opt == s_actions.end())
        return false;
    return status_opt->second.value;
}

void Input::set_action_value(std::string_view action, float value)
{
    auto& status = s_actions[std::string(action)];

    if (status.value > 0.0f && value == 0.0f)
        status.released = true;

    status.value = value;

    if (value == 0.0f)
        status.repeat = false;
}

glm::vec2 Input::get_vector(std::string_view x_negative, std::string_view x_positive, std::string_view y_negative, std::string_view y_positive)
{
    return glm::vec2(get_action_value(x_positive) - get_action_value(x_negative), get_action_value(y_positive) - get_action_value(y_negative));
}

float Input::get_axis(std::string_view negative, std::string_view positive)
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

glm::i32vec2 Input::get_mouse_coordinates()
{
    float x, y;
    SDL_GetMouseState(&x, &y);

    return {int32_t(x), int32_t(y)};
}

void Input::post_events()
{
    s_mouse_relative = glm::vec2();

    for (auto& [key, status] : s_actions)
    {
        status.released = false;

        if (status.value > 0)
            status.repeat = true;

        if (s_mappings[key][0].kind == ActionMappingKind::Wheel)
        {
            status.repeat = false;
            status.value = 0.0;
        }
    }
}

void Input::process_event(SDL_Event event)
{
    for (const auto& [key, value] : s_mappings)
    {
        for (const auto& mapping : value)
        {
            if (event.type == SDL_EVENT_KEY_DOWN && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(key, 1.0);
            }
            else if (event.type == SDL_EVENT_KEY_UP && mapping.kind == ActionMappingKind::Key && event.key.key == mapping.value)
            {
                set_action_value(key, 0.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(key, 1.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && mapping.kind == ActionMappingKind::MouseButton && event.button.button == mapping.value)
            {
                set_action_value(key, 0.0);
            }
            else if (event.type == SDL_EVENT_MOUSE_WHEEL && mapping.kind == ActionMappingKind::Wheel)
            {
                if (event.wheel.y < 0)
                    set_action_value(key, 1.0);
                else
                    set_action_value(key, -1.0);
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        s_mouse_relative.x = event.motion.xrel;
        s_mouse_relative.y = event.motion.yrel;
    }
}

void Input::add_action(std::string_view name)
{
    s_mappings[std::string(name)] = {};
}

void Input::add_action_mapping(std::string_view name, ActionMapping mapping)
{
    std::vector<ActionMapping>& mappings = s_mappings.find(name)->second;
    mappings.push_back(mapping);
}
