#include "Window.hpp"

Window::Window(const std::string& title, uint32_t width, uint32_t height, bool resizable)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    if (resizable)
        flags |= SDL_WINDOW_RESIZABLE;

    m_window = SDL_CreateWindow(title.c_str(), (int)width, (int)height, flags);
}

Window::~Window()
{
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

Extent2D Window::size() const
{
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return Extent2D((uint32_t)w, (uint32_t)h);
}

std::optional<SDL_Event> Window::poll_event() const
{
    SDL_Event event;

    if (SDL_PollEvent(&event))
        return event;
    else
        return std::nullopt;
}

void Window::set_fullscreen(bool f)
{
    SDL_SetWindowFullscreen(m_window, f);
}

void Window::close()
{
    m_running = false;
}
