#include "Args.hpp"
#include "Console.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Player.hpp"
#include "Font.hpp"
#include "Render/Driver.hpp"
#include "Toml.hpp"
#include "Window.hpp"
#include "World/Generator.hpp"
#include "World/Pass/Surface.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

#ifndef DOCTEST_CONFIG_ENABLE

static void register_all_classes();

static std::string default_config = R"(
[player]
x=0.0
y=150.0
z=0.0
)";

Ref<Window> window;
Ref<Entity> player;
Ref<World> world;

toml::table config2;
Console console;

ENGINE_MAIN(int argc, char *argv[])
{
    Args args = default_args();
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("connect-to", {.type = ArgType::String});
    args.parse(argv, argc);

    Engine engine(args, "ft_minecraft");

    register_all_classes();

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
    camera->get_transform().position() = glm::vec3(0, 0.9, 0);

    player = newobj(Player);
    player->get_transform().position() = glm::vec3(0, 5.0, 0);
    player->add_child(camera);

    world->get_dimension(World::overworld).add_entity(player);
    world->set_active_camera(camera);
    engine.set_world(world);

    if (args.has("disable-save"))
    {
        // TODO
        info("World won't be saved, `--disable-save` is present.");
    }

    engine.set_shutdown_callback([](Engine *, void *)
                                 {
                                    //  glm::vec3 position = player->get_transform()->get_global_transform().position();
                                    //  config2["player"]["x"].value<float>().emplace(position.x);
                                    //  config2["player"]["y"].value<float>().emplace(position.y);
                                    //  config2["player"]["z"].value<float>().emplace(position.z);

                                     // config.save_to("config.ini");

                                     BlockRegistry::destroy();
                                     Font::deinit_library();

                                     RenderingDriver::destroy_singleton(); });
    engine.run();

    return 0;
}

static void register_all_classes()
{
    REGISTER_CLASSES(World, Chunk, Block, Player, Generator, GeneratorPass, SurfacePass);

    REGISTER_STRUCTS(BlockState);
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
