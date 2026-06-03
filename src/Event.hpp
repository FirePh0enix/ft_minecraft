#pragma once

#include <SDL3/SDL.h>

struct Event
{
    Event(SDL_Event event)
        : event(event)
    {
    }

    SDL_Event event;
    bool handled = false;

    void handle() { handled = false; }
};
