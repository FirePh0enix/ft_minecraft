#pragma once

#include "Core/Class.hpp"
#include "Core/Types.hpp"

#include <SDL3/SDL.h>

#include <optional>

class Window : public Object
{
    CLASS(Window, Object);

public:
    Window()
    {
    }

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
