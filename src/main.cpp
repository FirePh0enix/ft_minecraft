#include "Args.hpp"
#include "Console.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Core/Registry.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "MeshPrimitives.hpp"
#include "Render/Driver.hpp"
#include "Render/Graph.hpp"
#include "Render/Shader.hpp"
#include "Render/WebGPU/DriverWebGPU.hpp"
#include "Scene/Components/MeshInstance.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transformed3D.hpp"
#include "Scene/Components/Visual.hpp"
#include "Scene/Player.hpp"
#include "Scene/Scene.hpp"
#include "Toml.hpp"
#include "Window.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#ifdef __platform_web

#define MAIN_FUNC_NAME emscripten_main
#define MAIN_ATTRIB __attribute__((used, visibility("default"))) extern "C"

#else

#define MAIN_FUNC_NAME main
#define MAIN_ATTRIB

#endif

#ifndef DOCTEST_CONFIG_ENABLE

static void register_all_classes();
static void tick();
static void main_loop();

static std::string default_config = R"(
[physics]
collisions=true
gravity=true
gravity_value=9.81

[window]
width=1280
height=720

[player]
x=0.0
y=50.0
z=0.0
)";

Ref<Window> window;
Ref<Scene> scene;
Ref<Entity> player;
Ref<World> world;

// Config config;
toml::table config2;
Console console;

MAIN_ATTRIB int MAIN_FUNC_NAME(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    initialize_error_handling(argv[0]);
#endif

    Args args;
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});

    args.parse(argv, argc);

    toml::table default_config2 = toml::operator""_toml(default_config.data(), default_config.size()).table();
    config2 = default_config2;
    if (std::filesystem::exists("config.toml"))
        toml_merge_left(config2, toml::parse_file("config.toml").table());

    tracy::SetThreadName("Main");

    register_all_classes();

    Filesystem::init();

    window = newobj(Window, "ft_minecraft", config2["window"]["width"].as<int64_t>()->get(), config2["window"]["height"].as<int64_t>()->get());
    Input::init(*window);
    Input::load_config();

    RenderingDriver::create_singleton<RenderingDriverWebGPU>();

    auto init_result = RenderingDriver::get()->initialize(*window, args.has("enable-gpu-validation"));
    EXPECT(init_result);

    BlockRegistry::load_blocks();
    BlockRegistry::create_gpu_resources();

    Result<Ref<Shader>> shader_result = Shader::load("assets/shaders/voxel.slang");
    if (!shader_result.has_value())
    {
        Result<> error = Error(ErrorKind::ShaderCompilationFailed);
        EXPECT(error);
    }

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

    scene->add_system(EarlyUpdate, [](const Query<One<Camera>>& query, Action&)
                      { query.template get<0>().single().template get<Camera>()->update_frustum(); });

    scene->add_system(Update, [](const Query<Many<Transformed3D, RigidBody>>& query, Action&)
                      {
                                                    for (const auto& result : query.template get<0>().results())
                                                    {
                                                        result.template get<Transformed3D>()->get_transform().position() = result.template get<RigidBody>()->get_body()->get_position();
                                                    } });

    scene->add_system(Update, Player::update);

    scene->add_system(LateUpdate, [](const Query<Many<VisualComponent>>& query, Action&)
                      {
                            // depth pass
                            {
                                RenderPassEncoder encoder = RenderGraph::get().render_pass_begin({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
                                for (const auto& result : query.template get<0>().results())
                                {
                                    result.template get<VisualComponent>()->encode_draw_calls(encoder, *Scene::get_active_scene()->get_active_camera());
                                }
                            }
                            // color pass
                            {
                                RenderPassEncoder encoder = RenderGraph::get().render_pass_begin({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
                                for (const auto& result : query.template get<0>().results())
                                {
                                    result.template get<VisualComponent>()->encode_draw_calls(encoder, *Scene::get_active_scene()->get_active_camera());
                                }
                                encoder.imgui();
                            } });

    world = newobj(World, cube, material, 1);
    world->set_render_distance(3);

    Ref<Entity> world_entity = newobj(Entity);
    world_entity->add_component(world);

    scene->add_entity(world_entity);

    Ref<Camera> camera = newobj(Camera);

    Ref<Entity> player_head = newobj(Entity);
    player_head->add_component(newobj(Transformed3D, glm::vec3(0.0, 0.85, 0.0)));
    player_head->add_component(camera);

    player = newobj(Entity);
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

    player->get_component<RigidBody>()->set_disabled(!config2["physics"]["collisions"].as<bool>()->get());
    player->get_component<Player>()->set_gravity_enabled(config2["physics"]["gravity"].as<bool>()->get());
    player->get_component<Player>()->set_gravity_value(static_cast<float>(config2["physics"]["gravity_value"].as<double>()->get()));

    console.register_command("tp", {CmdArgInfo(CmdArgKind::Int, "x"), CmdArgInfo(CmdArgKind::Int, "y"), CmdArgInfo(CmdArgKind::Int, "z")}, [](const Command& cmd)
                             { player->get_component<Transformed3D>()->get_transform().position() = glm::vec3(cmd.get_arg_int("x"), cmd.get_arg_int("y"), cmd.get_arg_int("z")); });

    EXPECT(RenderingDriver::get()->initialize_imgui(*window));

    const glm::vec3 player_pos = player->get_transform()->get_global_transform().position();
    world->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));

#ifdef __platform_web
    emscripten_set_main_loop_arg([](void *)
                                 { main_loop(); }, nullptr, 0, true);
#else
    while (window->is_running())
    {
        main_loop();
    }
#endif

#ifndef __platform_web
    info("Shutting down...");

    glm::vec3 position = player->get_transform()->get_global_transform().position();
    config2["player"]["x"].value<float>().emplace(position.x);
    config2["player"]["y"].value<float>().emplace(position.y);
    config2["player"]["z"].value<float>().emplace(position.z);

    // config.save_to("config.ini");

    // (void)RenderingDriverVulkan::get()->get_device().waitIdle();

    BlockRegistry::destroy();
    Font::deinit_library();

    // RenderingDriver::destroy_singleton();
#endif

    return 0;
}

clock_t last_tick_time = 0;
constexpr float time_between_ticks = 1.0 / 60.0;

static void main_loop()
{
    if ((float)(clock() - last_tick_time) / CLOCKS_PER_SEC >= time_between_ticks)
    {
        last_tick_time = clock();
        tick();
    }
}

static void tick()
{
    FrameMark;

    std::optional<SDL_Event> event;

    while ((event = window->poll_event()))
    {
        switch (event->type)
        {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            window->close();
        }
        break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
            Result<> result = RenderingDriver::get()->configure_surface(*window, VSync::Off);
            ERR_EXPECT_B(result, "Failed to configure the surface");

            // TODO
            // config.get_category("window").set("width", (int64_t)window->size().width);
            // config.get_category("window").set("height", (int64_t)window->size().height);
            // config.save_to("config.ini");
        }
        break;
        default:
            break;
        }

#ifdef __has_debug_menu
        ImGui_ImplSDL3_ProcessEvent(&*event);

        ImGuiIO& imgui_io = ImGui::GetIO();

        if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
        {
            continue;
        }
#endif

        Input::process_event(event.value());
    }

    RenderingDriver::get()->poll();

#ifdef __has_debug_menu
    {
        ZoneScopedN("tick.debug_menu");

        ImGui::NewFrame();

        // if (ImGui::Begin("Commands"))
        // {
        //     ImGui::InputText("##", console.get_buffer(), console.get_buffer_size(), ImGuiInputTextFlags_None);
        //     ImGui::SameLine();
        //     if (ImGui::Button(">"))
        //     {
        //         console.exec();
        //     }
        // }
        // ImGui::End();

        scene->imgui_debug_window();
    }
#endif

    const glm::vec3 player_pos = player->get_transform()->get_global_transform().position();
    world->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));

    Ref<Scene>& scene = Scene::get_active_scene();

    // TODO: Differentiate between rendering ticks and physics ticks.
    scene->tick(time_between_ticks);

    scene->run_systems(EarlyUpdate);
    scene->run_systems(Update);
    scene->run_systems(LateUpdate);

    scene->run_systems(EarlyFixedUpdate);
    scene->run_systems(FixedUpdate);
    scene->run_systems(LateFixedUpdate);

    Input::post_events();
    RenderingDriver::get()->draw_graph(RenderGraph::get());

    // Reset after drawing the graph, not before drawing so that `main` can add copy calls.
    RenderGraph::get().reset();
}

static void register_all_classes()
{
    REGISTER_CLASSES(
        Font,
        Entity,
        Component, VisualComponent, MeshInstance, RigidBody, Player, Camera, Transformed3D,
        World, Chunk, Block,
        RenderingDriver, Buffer, Texture, Mesh, MaterialBase, Material, ComputeMaterial, Shader);

#ifdef __has_webgpu
    REGISTER_CLASSES(RenderingDriverWebGPU, BufferWebGPU, TextureWebGPU);
#endif

    REGISTER_STRUCTS(
        uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::uvec3, glm::uvec4,
        BlockState, ChunkGPUInfo,
        StandardMeshMaterialInfo,
        SimplexState);
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
