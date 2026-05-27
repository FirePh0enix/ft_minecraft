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

    static bool is_action_pressed(const StringView& action);
    static bool is_action_just_pressed(const StringView& action);

    static float get_action_value(const StringView& action);

    static glm::vec2 get_vector(const StringView& x_negative, const StringView& x_positive, const StringView& y_negative, const StringView& y_positive);

    static void set_mouse_grabbed(bool value);

    static bool is_mouse_grabbed();

    static glm::vec2 get_mouse_relative();

    /**
     * Mouse coordinates in the same range as the UI with X in [-w/h, +w/h] and Y [-1, 1].
     */
    static glm::vec2 get_mouse_absolute();

    static void post_events();

    static void process_event(SDL_Event event);

    static void add_action(const StringView& name);
    static void add_action_mapping(const StringView& name, ActionMapping mapping);

private:
    Input()
    {
    }

    static void set_action_value(const StringView& action, float value);

    static inline HashMap<String, Vector<ActionMapping>> s_mappings;
    static inline HashMap<String, Status> s_actions;
    static inline bool s_mouse_grabbed;
    static inline glm::vec2 s_mouse_relative;

#ifndef __platform_web
    static inline SDL_Window *s_window;
#endif
};
