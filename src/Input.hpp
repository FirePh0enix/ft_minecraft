#pragma once

#include "Window.hpp"

#include <map>

struct Status
{
    float value = 0.0;
};

enum class ActionMappingKind
{
    Key,
    MouseButton,
};

struct ActionMapping
{
    ActionMappingKind kind;
    uint32_t value;

    ActionMapping(ActionMappingKind kind, uint32_t value)
        : kind(kind), value(value)
    {
    }
};

class Input
{
public:
    static void init(const Window& window);
    static void load_config();

    static bool is_action_pressed(const std::string& action);
    static float get_action_value(const std::string& action);

    static glm::vec2 get_vector(const std::string& x_negative, const std::string& x_positive, const std::string& y_negative, const std::string& y_positive);

    static void set_mouse_grabbed(bool value);

    static bool is_mouse_grabbed();

    static glm::vec2 get_mouse_relative();

    static void post_events();

    static void process_event(SDL_Event event);

    static void add_action(const std::string& name);
    static void add_action_mapping(const std::string& name, ActionMapping mapping);

private:
    Input()
    {
    }

    static void set_action_value(const std::string& action, float value);

    static inline std::map<std::string, std::vector<ActionMapping>> s_mappings;
    static inline std::map<std::string, Status> s_actions;
    static inline bool s_mouse_grabbed;
    static inline glm::vec2 s_mouse_relative;

#ifndef __platform_web
    static inline SDL_Window *s_window;
#endif
};
