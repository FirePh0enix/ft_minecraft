#include "Args.hpp"
#include "Config.hpp"
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
#include "Window.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#ifdef __has_debug_menu
#include <imgui.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
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
Ref<Entity> player;
Ref<World> world;

Config config;

MAIN_ATTRIB int MAIN_FUNC_NAME(int argc, char *argv[])
{
#if !defined(__has_address_sanitizer) || !defined(__platform_web)
    initialize_error_handling(argv[0]);
#endif

    Args args;
    args.add_arg("disable-save", {.type = ArgType::Bool});
    args.add_arg("enable-gpu-validation", {.type = ArgType::Bool});

    args.parse(argv, argc);

    config.load_from_str(default_config);
    config.load_from_file("config.ini");

    tracy::SetThreadName("Main");

    register_all_classes();

    Filesystem::init();

    window = make_ref<Window>("ft_minecraft", config.get_category("window").get<int64_t>("width"), config.get_category("window").get<int64_t>("height"));
    Input::init(*window);

    Input::add_action("forward");
    Input::add_action_mapping("forward", ActionMapping(ActionMappingKind::Key, SDLK_W));

    Input::add_action("backward");
    Input::add_action_mapping("backward", ActionMapping(ActionMappingKind::Key, SDLK_S));

    Input::add_action("left");
    Input::add_action_mapping("left", ActionMapping(ActionMappingKind::Key, SDLK_A));

    Input::add_action("right");
    Input::add_action_mapping("right", ActionMapping(ActionMappingKind::Key, SDLK_D));

    Input::add_action("up");
    Input::add_action_mapping("up", ActionMapping(ActionMappingKind::Key, SDLK_SPACE));

    Input::add_action("down");
    Input::add_action_mapping("down", ActionMapping(ActionMappingKind::Key, SDLK_LCTRL));

    Input::add_action("attack");
    Input::add_action_mapping("attack", ActionMapping(ActionMappingKind::MouseButton, SDL_BUTTON_LEFT));

    Input::add_action("place");
    Input::add_action_mapping("place", ActionMapping(ActionMappingKind::MouseButton, SDL_BUTTON_RIGHT));

    Input::add_action("escape");
    Input::add_action_mapping("escape", ActionMapping(ActionMappingKind::Key, SDLK_ESCAPE));

    RenderingDriver::create_singleton<RenderingDriverWebGPU>();

    auto init_result = RenderingDriver::get()->initialize(*window, args.has("enable-gpu-validation"));
    EXPECT(init_result);

    Result<Ref<Shader>> shader_result = Shader::load("assets/shaders/voxel.slang");
    if (!shader_result.has_value())
    {
        Result<> error = Error(ErrorKind::ShaderCompilationFailed);
        EXPECT(error);
    }

    Ref<Shader> shader = shader_result.value();
    shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});

    // std::array<InstanceLayoutInput, 3> inputs{
    //     InstanceLayoutInput(ShaderType::Float32x3, 0),
    //     InstanceLayoutInput(ShaderType::Uint32x3, sizeof(glm::vec3) * 1),
    //     InstanceLayoutInput(ShaderType::Uint32, sizeof(glm::vec3) * 2),
    // };
    // InstanceLayout instance_layout(inputs, sizeof(BlockInstanceData));
    Ref<Material> material = Material::create(shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    // TODO: Add this back
    // Font::init_library();

    // auto font_result = Font::create("assets/fonts/Anonymous.ttf", 20);
    // EXPECT(font_result);
    // Ref<Font> font = font_result.value();

    Ref<Scene> scene = make_ref<Scene>();
    Scene::set_active_scene(scene);

    world = make_ref<World>(cube, material);
    world->set_render_distance(3);

    Ref<Entity> world_entity = make_ref<Entity>();
    world_entity->add_component(world);

    scene->add_entity(world_entity);

    Ref<Camera> camera = make_ref<Camera>();

    Ref<Entity> player_head = make_ref<Entity>();
    player_head->add_component(make_ref<Transformed3D>(glm::vec3(0.0, 0.85, 0.0)));
    player_head->add_component(camera);

    player = make_ref<Entity>();
    player->add_component(make_ref<Transformed3D>(Transform3D(glm::vec3(config.get_category("player").get<float>("x"), config.get_category("player").get<float>("y"), config.get_category("player").get<float>("z")))));
    player->add_component(make_ref<RigidBody>(AABB(glm::vec3(), glm::vec3(0.3, 0.9, 0.3))));
    player->add_component(make_ref<Player>(world, cube));
    player->add_child(player_head);

    scene->add_entity(player);
    scene->set_active_camera(camera);

    BlockRegistry::load_blocks();

    BlockRegistry::create_texture_array();
    material->set_param("images", BlockRegistry::get_texture_array());

    if (args.has("disable-save"))
    {
        info("World won't be saved, `--disable-save` is present.");
    }

    // gen = make_ref<WorldGenerator>(world, args.has("disable-save"));
    // gen->set_terrain(make_ref<OverworldTerrainGenerator>());

    player->get_component<RigidBody>()->disabled() = !config.get_category("physics").get<bool>("collisions");
    player->get_component<Player>()->set_gravity_enabled(config.get_category("physics").get<bool>("gravity"));
    player->get_component<Player>()->set_gravity_value(config.get_category("physics").get<float>("gravity_value"));

    // This is very hacky but the only way to create the render pass required for ImGui.
    // TODO: Maybe a solution would be to create a render pass only for imgui ?
    {
        RenderGraph& graph = RenderGraph::get();

        // depth prepass
        {
            graph.render_pass_begin({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
        }

        // main color pass
        {
            graph.render_pass_begin({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
        }

        RenderingDriver::get()->draw_graph(graph);
        graph.reset();

        EXPECT(RenderingDriver::get()->initialize_imgui());
    }

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
    config.get_category("player").set("x", position.x);
    config.get_category("player").set("y", position.y);
    config.get_category("player").set("z", position.z);

    config.save_to("config.ini");

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

            config.get_category("window").set("width", (int64_t)window->size().width);
            config.get_category("window").set("height", (int64_t)window->size().height);
            config.save_to("config.ini");
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

        if (ImGui::Begin("Physics"))
        {
            bool collisions = !player->get_component<RigidBody>()->disabled();
            if (ImGui::Checkbox("Enable collisions", &collisions))
            {
                config.get_category("physics").set("collisions", collisions);
                player->get_component<RigidBody>()->disabled() = !collisions;
            }

            bool gravity = player->get_component<Player>()->is_gravity_enabled();
            if (ImGui::Checkbox("Enable gravity", &gravity))
            {
                config.get_category("physics").set("gravity", gravity);
                player->get_component<Player>()->set_gravity_enabled(gravity);
            }

            float gravity_value = player->get_component<Player>()->get_gravity_value();
            if (ImGui::SliderFloat("Gravity value", &gravity_value, 0.0, 20.0))
            {
                config.get_category("physics").set("gravity_value", gravity_value);
                player->get_component<Player>()->set_gravity_value(gravity_value);
            }
        }
        ImGui::End();

        if (ImGui::Begin("General info"))
        {
            glm::vec3 pos = player->get_transform()->get_transform().position();
            ImGui::Text("X: %f", pos.x);
            ImGui::Text("Y: %f", pos.y);
            ImGui::Text("Z: %f", pos.z);
        }
        ImGui::End();
    }
#endif

    // const glm::vec3 player_pos = player->get_transform()->get_global_transform().position();
    // world->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));
    // gen->set_player_pos(player_pos);
    // gen->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));

    Ref<Scene>& scene = Scene::get_active_scene();

    scene->tick();

    Input::post_events();

    RenderGraph& graph = RenderGraph::get();

    // depth prepass
    {
        RenderPassEncoder encoder = graph.render_pass_begin({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
        scene->encode_draw_calls(encoder);
    }

    // main color pass
    {
        RenderPassEncoder encoder = graph.render_pass_begin({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
        scene->encode_draw_calls(encoder);

        // graph.add_imgui_draw();
    }

    RenderingDriver::get()->draw_graph(graph);

    // Reset after drawing the graph, not before drawing so that `main` can add copy calls.
    graph.reset();
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
        uint16_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4,
        BlockState, ChunkGPUInfo,
        StandardMeshMaterialInfo,
        SimplexState);
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_ENABLE
