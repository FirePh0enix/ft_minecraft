#include "Input.hpp"
#include "Toml.hpp"

void Input::init(const Window& window)
{
#ifndef __platform_web
    s_window = window.get_window_ptr();
#endif
}

static uint32_t convert_key(const std::string_view& key)
{
    if (key == "lctrl")
        return SDLK_LCTRL;
    else if (key == "rctrl")
        return SDLK_RCTRL;
    else if (key == "escape")
        return SDLK_ESCAPE;
    else if (key == "space")
        return SDLK_SPACE;
    return key[0];
}

void Input::load_config()
{
    toml::table table = toml::parse_file("inputs.toml").table();
    toml::table *actions = table["actions"].as_table();

    actions->for_each([](const auto& key, const auto& value)
                      {
                          const std::string action_name = std::string(key.str());
                          Input::add_action(action_name);
                          const toml::array *array = value.as_array();
                        array->for_each([action_name](const auto& value) {

                            const toml::table *action = value.as_table();
                            const std::string_view action_type = action->get("type")->as_string()->get();
                            if (action_type == "key")
                            {
                                const std::string_view key = action->get("key")->as_string()->get();
                                Input::add_action_mapping(action_name, ActionMapping(ActionMappingKind::Key, convert_key(key)));
                            }
                            else if (action_type == "mouse_button")
                            {
                                const int64_t button = action->get("button")->as_integer()->get();
                                Input::add_action_mapping(action_name, ActionMapping(ActionMappingKind::MouseButton, button));
                            }
                        }); });
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
