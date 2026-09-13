#pragma once

#include "Input.hpp"

#include <SDL3/SDL.h>

#include <string_view>

struct Event
{
    Event(SDL_Event event)
        : event(event)
    {
    }

    SDL_Event event;
    bool handled = false;

    void handle() { handled = false; }

    bool is_action_pressed(std::string_view action)
    {
        return (event.type == SDL_EVENT_KEY_DOWN && Input::is_action_mapping(action, event.key.key)) || (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && Input::is_action_mapping(action, event.button.button));
    }

    bool is_action_released(std::string_view action)
    {
        return (event.type == SDL_EVENT_KEY_UP && Input::is_action_mapping(action, event.key.key)) || (event.type == SDL_EVENT_MOUSE_BUTTON_UP && Input::is_action_mapping(action, event.button.button));
    }
};
