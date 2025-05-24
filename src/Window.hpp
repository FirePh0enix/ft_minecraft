#pragma once

#include "Core/Types.hpp"

#include <optional>

#include <SDL3/SDL.h>

class Window
{
public:
    Window(const std::string& title, uint32_t width, uint32_t height, bool resizable = true);
    ~Window();

    Extent2D size() const;

    [[nodiscard]]
    std::optional<SDL_Event> poll_event() const;

    void set_fullscreen(bool f);
    void close();

    inline bool is_running() const
    {
        return m_running;
    }

    inline SDL_Window *get_window_ptr() const
    {
        return m_window;
    }

private:
    SDL_Window *m_window;
    bool m_running = true;
};
