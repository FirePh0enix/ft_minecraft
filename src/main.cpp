#include "Args.hpp"
#include "Console.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Player.hpp"
#include "Font.hpp"
#include "MeshPrimitives.hpp"
#include "Render/Driver.hpp"
#include "Render/Shader.hpp"
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

    Result<Ref<Shader>> shader_result = Shader::load("assets://shaders/voxel.slang");
    if (!shader_result.has_value())
        EXPECT(Result<>(Error(ErrorKind::ShaderCompilationFailed)));

    Ref<Shader> shader = shader_result.value();
    shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    // TODO: Add this back
    // Font::init_library();

    // auto font_result = Font::create("assets://fonts/Anonymous.ttf", 20);
    // EXPECT(font_result);
    // Ref<Font> font = font_result.value();

    world = newobj(World, cube, shader, 1);

    Ref<Camera> camera = newobj(Camera);
    camera->get_transform().position() = glm::vec3(0, 0.9, 0);

    player = newobj(Player);
    player->get_transform().position() = glm::vec3(0, 70.0, 0);
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
