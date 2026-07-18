#include "Window.hpp"
#include "Core/Logger.hpp"

#include <SDL3/SDL.h>

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

Window::Window(const String& title, uint32_t width, uint32_t height, bool resizable)
{
#ifndef __platform_web
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        error("SDL_Init() failed: {}", SDL_GetError());
        return;
    }

    SDL_WindowFlags flags = 0;

#if defined(__platform_linux) || defined(__platform_windows)
    flags |= SDL_WINDOW_VULKAN;
#elif defined(__platform_macos)
    flags |= SDL_WINDOW_METAL;
#endif

    // flags |= SDL_WINDOW_RESIZABLE;

    int display_count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays(&display_count);
    assert(display_count > 0);

    const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode(displays[0]);
    constexpr float factor = 0.9;
    int w = float(dm->w) * factor;
    int h = float(dm->h) * factor;

    m_window = SDL_CreateWindow(title.data(), w, h, flags);

    if (m_window == nullptr)
    {
        error("SDL_CreateWindow() failed: {}", SDL_GetError());
        return;
    }

    SDL_StartTextInput(m_window);
#else
    if (!SDL_Init(SDL_INIT_EVENTS))
    {
        error("SDL_Init() failed: {}", SDL_GetError());
        return;
    }
#endif
}

Window::~Window()
{
    SDL_StopTextInput(m_window);
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

Option<SDL_Event> Window::poll_event() const
{
    SDL_Event event;

    if (SDL_PollEvent(&event))
        return event;
    else
        return None;
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
