#pragma once

#include "Window.hpp"

struct Status
{
    float value = 0.0;
    bool repeat = false;
};

enum class ActionMappingKind
{
    Key,
    MouseButton,
    Wheel,
};

struct ActionMapping
{
    ActionMappingKind kind;
    uint32_t value;

    ActionMapping()
        : kind(ActionMappingKind::Key), value(0)
    {
    }

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

    static bool is_action_pressed(std::string_view action);
    static bool is_action_just_pressed(std::string_view action);

    static float get_action_value(std::string_view action);

    static float get_axis(std::string_view negative, std::string_view positive);
    static glm::vec2 get_vector(std::string_view x_negative, std::string_view x_positive, std::string_view y_negative, std::string_view y_positive);

    static void set_mouse_grabbed(bool value);

    static bool is_mouse_grabbed();

    static glm::vec2 get_mouse_relative();

    /**
     * Mouse coordinates in the same range as the UI with X in [-w/h, +w/h] and Y [-1, 1].
     */
    static glm::vec2 get_mouse_absolute();
    static glm::i32vec2 get_mouse_coordinates();

    static void post_events();

    static void process_event(SDL_Event event);

    static void add_action(std::string_view name);
    static void add_action_mapping(std::string_view name, ActionMapping mapping);

private:
    Input()
    {
    }

    static void set_action_value(std::string_view action, float value);

    static inline stdext::string_map<std::vector<ActionMapping>> s_mappings;
    static inline stdext::string_map<Status> s_actions;
    static inline bool s_mouse_grabbed;
    static inline glm::vec2 s_mouse_relative;

#ifndef __platform_web
    static inline SDL_Window *s_window;
#endif
};
