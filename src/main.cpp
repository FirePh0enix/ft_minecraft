#include "Args.hpp"
#include "Console.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Engine.hpp"
#include "Font.hpp"
#include "MP/Manager.hpp"
#include "MeshPrimitives.hpp"
#include "Render/Driver.hpp"
#include "Render/Shader.hpp"
#include "Scene/Components/MeshInstance.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transformed3D.hpp"
#include "Scene/Player.hpp"
#include "Scene/Scene.hpp"
#include "Toml.hpp"
#include "Window.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>
#include <tracy/Tracy.hpp>

#ifndef DOCTEST_CONFIG_ENABLE

static void register_all_classes();

static std::string default_config = R"(
[player]
x=0.0
y=50.0
z=0.0
)";

Ref<Window> window;
Ref<Scene> scene;
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

    Result<Ref<Shader>> shader_result = Shader::load("assets/shaders/voxel.slang");
    if (!shader_result.has_value())
        EXPECT(Result<>(Error(ErrorKind::ShaderCompilationFailed)));

    Ref<Shader> shader = shader_result.value();
    shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});

    Ref<Material> material = Material::create(shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    // TODO: Add this back
    // Font::init_library();

    // auto font_result = Font::create("assets/fonts/Anonymous.ttf", 20);
    // EXPECT(font_result);
    // Ref<Font> font = font_result.value();

    scene = newobj(Scene);
    Scene::set_active_scene(scene);

    scene->add_plugins<RenderingPlugin, PhysicsPlugin, MultiplayerPlugin>();
    scene->add_system(Update, Player::update);

    world = newobj(World, cube, material, 1);
    world->set_render_distance(3);

    Ref<Entity> world_entity = newobj(Entity);
    world_entity->set_name("World");
    world_entity->add_component(world);

    scene->add_entity(world_entity);

    Ref<Camera> camera = newobj(Camera);

    Ref<Entity> player_head = newobj(Entity);
    player_head->set_name("Head");
    player_head->add_component(newobj(Transformed3D, glm::vec3(0.0, 0.85, 0.0)));
    player_head->add_component(camera);

    player = newobj(Entity);
    player->set_name("Player");
    player->add_component(newobj(Transformed3D, Transform3D(glm::vec3(config2["player"]["x"].as<double>()->get(), config2["player"]["y"].as<double>()->get(), config2["player"]["z"].as<double>()->get()))));
    player->add_component(newobj(RigidBody));
    player->add_component(newobj(Player, world, cube));
    player->add_child(player_head);

    scene->add_entity(player);
    scene->set_active_camera(camera);

    material->set_param("images", BlockRegistry::get_texture_array());

    if (args.has("disable-save"))
    {
        // TODO
        info("World won't be saved, `--disable-save` is present.");
    }

    // player->get_component<RigidBody>()->set_disabled(!config2["physics"]["collisions"].as<bool>()->get());
    // player->get_component<Player>()->set_gravity_enabled(config2["physics"]["gravity"].as<bool>()->get());
    // player->get_component<Player>()->set_gravity_value(static_cast<float>(config2["physics"]["gravity_value"].as<double>()->get()));

    console.register_command("tp", {CmdArgInfo(CmdArgKind::Int, "x"), CmdArgInfo(CmdArgKind::Int, "y"), CmdArgInfo(CmdArgKind::Int, "z")}, [](const Command& cmd)
                             { player->get_component<Transformed3D>()->get_transform().position() = glm::vec3(cmd.get_arg_int("x"), cmd.get_arg_int("y"), cmd.get_arg_int("z")); });

    engine.set_shutdown_callback([](Engine *, void *)
                                 {
                                     glm::vec3 position = player->get_transform()->get_global_transform().position();
                                     config2["player"]["x"].value<float>().emplace(position.x);
                                     config2["player"]["y"].value<float>().emplace(position.y);
                                     config2["player"]["z"].value<float>().emplace(position.z);

                                     // config.save_to("config.ini");

                                     // (void)RenderingDriverVulkan::get()->get_device().waitIdle();

                                     BlockRegistry::destroy();
                                     Font::deinit_library();

                                     // RenderingDriver::destroy_singleton();
                                 },
                                 nullptr);
    engine.run();

    return 0;
}

static void register_all_classes()
{
    REGISTER_CLASSES(World, Chunk, Block);

    REGISTER_STRUCTS(
        BlockState, ChunkGPUInfo,
        StandardMeshMaterialInfo,
        SimplexState);
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
