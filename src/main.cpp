#include "Args.hpp"
#include "Console.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Player.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/Driver.hpp"
#include "Toml.hpp"
#include "Window.hpp"
#include "World/Generator.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include <SDL3_image/SDL_image.h>

#ifdef __has_webgpu
#include "Render/WebGPU/DriverWebGPU.hpp"
#endif

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#ifdef __platform_web
#define MAIN(...) __attribute__((used, visibility("default"))) extern "C" int emscripten_main(__VA_ARGS__)
#else
#define MAIN(...) int main(__VA_ARGS__)
#endif

#ifndef DOCTEST_CONFIG_ENABLE

static constexpr double fixed_update_time = 1.0 / 60.0;
static clock_t last_update_time;

static void register_engine_classes();
static void register_all_classes();

static void shutdown_callback();
static void loop_update();
static void update_callback();

static std::string default_config = R"(
[player]
x=0.0
y=150.0
z=0.0
)";

Ref<Engine> engine;

Ref<Entity> player;
Ref<World> world;

toml::table config2;
Console console;

MAIN(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    // NOTE: Address sanitizer mess with our custom error handling.
    initialize_error_handling(Filesystem::current_executable_path().c_str());
#endif

    Args args;
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("connect-to", {.type = ArgType::String});
    args.parse(argv, argc);

    register_engine_classes();
    register_all_classes();

    engine = newobj(Engine, args);

    TracySetThreadName("Main");

    toml::table default_config2 = toml::operator""_toml(default_config.data(), default_config.size()).table();
    config2 = default_config2;
    if (std::filesystem::exists("config.toml"))
        toml_merge_left(config2, toml::parse_file("config.toml").table());

    BlockRegistry::load_blocks();
    BlockRegistry::create_gpu_resources();

    // TODO: Add this back
    // Font::init_library();

    // auto font_result = Font::create("assets/fonts/Anonymous.ttf", 20);
    // EXPECT(font_result);
    // Ref<Font> font = font_result.value();

    uint64_t seed = 1;
    world = newobj(World, seed);

    Ref<Camera> camera = newobj(Camera);
    camera->get_transform().position() = glm::vec3(0, 0.85, 0);

    player = newobj(Player);
    player->get_transform().position() = glm::vec3(0, 15.0, 0);
    player->add_child(camera);

    world->add_entity(World::overworld, player);
    world->set_active_camera(camera);

    if (args.has("disable-save"))
    {
        // TODO
        info("World won't be saved, `--disable-save` is present.");
    }

#ifdef __platform_web
    emscripten_set_main_loop_arg([](void *engine)
                                 { ((Engine *)engine)->update_callback(); }, nullptr, 0, true);
#else
    while (engine->is_running())
        loop_update();

    shutdown_callback();
#endif

    return 0;
}

static void loop_update()
{
    const float elapsed_time = (float)(clock() - last_update_time) / CLOCKS_PER_SEC;
    if (elapsed_time >= fixed_update_time)
    {
        last_update_time = clock();
        update_callback();
    }
}

static void update_callback()
{
    FrameMark;
    ZoneScoped;

    // TODO: Differentiate between rendering ticks and physics ticks.
    engine->tick(float(fixed_update_time));

    ImGui::NewFrame();
    engine->draw();

    Input::post_events();
}

static void shutdown_callback()
{
    println("Shutting down");

    // this stop the generator threads, needs to be done before destroying the RenderingDriver or it causes segmentation faults.
    world = nullptr;

    engine = nullptr;

    BlockRegistry::destroy();
    // Font::deinit_library();

    RenderingDriver::destroy_singleton();
}

static void register_all_classes()
{
    REGISTER_CLASSES(World, Chunk, Block, Player, Generator, GenerationPass, BiomeGenerationPass, SurfaceGenerationPass, FeaturesGenerationPass);

    REGISTER_STRUCTS(BlockState, Environment, Model);
}

static void register_engine_classes()
{
    REGISTER_CLASSES(
        Font,
        Entity, Camera,
        RenderingDriver, Buffer, Texture, Mesh, MaterialBase, Material, ComputeMaterial, Shader);

#ifdef __has_webgpu
    REGISTER_CLASSES(RenderingDriverWebGPU, BufferWebGPU, TextureWebGPU);
#endif

    REGISTER_STRUCTS(
        uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4);
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
