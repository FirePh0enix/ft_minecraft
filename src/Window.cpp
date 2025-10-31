#include "Window.hpp"

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

Window::Window(const std::string& title, uint32_t width, uint32_t height, bool resizable)
{
#ifndef __platform_web
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_WindowFlags flags = 0;

#ifdef __platform_linux
    flags |= SDL_WINDOW_VULKAN;
#elif defined(__platform_macos)
    flags |= SDL_WINDOW_METAL;
#elif defined(__platform_windows)
#error "Windows not implemented"
#endif

    if (resizable)
        flags |= SDL_WINDOW_RESIZABLE;

    m_window = SDL_CreateWindow(title.c_str(), (int)width, (int)height, flags);
#else
    SDL_Init(SDL_INIT_EVENTS);
#endif
}

Window::~Window()
{
#ifndef __platform_web
    SDL_DestroyWindow(m_window);
#endif
    SDL_Quit();
}

Extent2D Window::size() const
{
#ifdef __platform_web
    int width;
    int height;
    emscripten_get_canvas_element_size("#canvas", &width, &height);
    return Extent2D(width, height);
#else
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return Extent2D((uint32_t)w, (uint32_t)h);
#endif
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
#ifndef __platform_web
    SDL_SetWindowFullscreen(m_window, f);
#endif
}

void Window::close()
{
    m_running = false;
}
