#pragma once

#include "Args.hpp"
#include "Window.hpp"

class Engine;

struct EngineCallback
{
    void (*callback)(Engine *engine, void *user) = nullptr;
    void *user = nullptr;
};

class Engine
{
public:
    Engine(Args args, const std::string& app_name);

    /**
     * Start the engine loop.
     */
    void run();

    /**
     * Register a callback called when the engine will exit.
     */
    void set_shutdown_callback(void (*callback)(Engine *engine, void *user), void *user = nullptr) { m_shutdown_callback = EngineCallback(callback, user); }

    const Args& args() const { return m_args; }
    Args& args() { return m_args; }

private:
    Args m_args;
    std::string m_app_name;
    Ref<Window> m_window;
    clock_t m_last_tick_time = 0;

    EngineCallback m_shutdown_callback;

    double m_fixed_update_time = 1.0 / 60.0;

    void iterate();
    void update_callback();
};

Args default_args();

#ifdef __platform_web
#define ENGINE_MAIN(...) __attribute__((used, visibility("default"))) extern "C" int emscripten_main(__VA_ARGS__)
#else
#define ENGINE_MAIN(...) int main(__VA_ARGS__)
#endif
