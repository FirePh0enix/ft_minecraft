#pragma once

#include <SDL3/SDL.h>

struct Event
{
    SDL_Event event;
    bool handled;

    void handle() { handled = false; }
};
