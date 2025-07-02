#include "Args.hpp"
#include "Core/Logger.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "MeshPrimitives.hpp"
#include "Platform/Platform.hpp"
#include "Render/Driver.hpp"
#include "Render/Graph.hpp"
#include "Render/Shader.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transform3D.hpp"
#include "Scene/Components/Visual.hpp"
#include "Scene/Player.hpp"
#include "Scene/Scene.hpp"
#include "Window.hpp"
#include "World/Generation/Generator.hpp"
#include "World/Generation/Terrain.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

#ifdef __has_vulkan
#include "Render/Vulkan/DriverVulkan.hpp"
#endif

#ifdef __has_webgpu
#include "Render/WebGPU/DriverWebGPU.hpp"
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

#ifdef DOCTEST_CONFIG_DISABLE

static void register_all_classes();
static void tick();
static void main_loop();

Ref<Window> window;
Text text;

Ref<WorldGenerator> gen;
Ref<Entity> player;

MAIN_ATTRIB int MAIN_FUNC_NAME(int argc, char *argv[])
{
#ifndef __has_address_sanitizer
    initialize_error_handling(argv[0]);
#endif

    Args args;
    args.add_arg("disable-save", {.type = ArgType::Bool});

    args.parse(argv, argc);

    tracy::SetThreadName("Main");

    register_all_classes();

    static const int width = 1280;
    static const int height = 720;

    window = make_ref<Window>("ft_minecraft", width, height);

#ifdef __platform_web
    RenderingDriver::create_singleton<RenderingDriverWebGPU>();
#else
    RenderingDriver::create_singleton<RenderingDriverVulkan>();
#endif

    auto init_result = RenderingDriver::get()->initialize(*window);
    EXPECT(init_result);

    auto shader_result = Shader::compile("assets/shaders/voxel.wgsl", {}, {.vertex = true, .fragment = true});
    if (!shader_result.has_value())
    {
        Result<> error = Error(ErrorKind::ShaderCompilationFailed);
        EXPECT(error);
    }

    Ref<Shader> shader = shader_result.value();
#ifdef __platform_web
    shader->set_binding("images", Binding{.kind = BindingKind::Texture, .shader_stage = ShaderStageKind::Fragment, .group = 0, .binding = 0, .dimension = TextureDimension::D2DArray});
#endif

    shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});

    std::array<InstanceLayoutInput, 3> inputs{
        InstanceLayoutInput(ShaderType::Float32x3, 0),
        InstanceLayoutInput(ShaderType::Uint32x3, sizeof(glm::vec3) * 1),
        InstanceLayoutInput(ShaderType::Uint32, sizeof(glm::vec3) * 2),
    };
    InstanceLayout instance_layout(inputs, sizeof(BlockInstanceData));
    auto material_layout_result = RenderingDriver::get()->create_material_layout(shader, {.transparency = true}, instance_layout, CullMode::Back, PolygonMode::Fill);
    EXPECT(material_layout_result);
    Ref<MaterialLayout> material_layout = material_layout_result.value();

    auto material_result = RenderingDriver::get()->create_material(material_layout);
    EXPECT(material_result);
    Ref<Material> material = material_result.value();

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    Font::init_library();

    auto font_result = Font::create("assets/fonts/Anonymous.ttf", 20);
    EXPECT(font_result);
    Ref<Font> font = font_result.value();

    // text = Text("Hello world", font);
    // text.set_scale(glm::vec2(0.2, 0.2));
    // text.set_color(glm::vec4(1.0, 1.0, 1.0, 1.0));

    Ref<Scene> scene = make_ref<Scene>();
    Scene::set_active_scene(scene);

    Ref<World> world = make_ref<World>(cube, material);
    world->set_render_distance(16);

    Ref<Entity> world_entity = make_ref<Entity>();
    world_entity->add_component(world);

    scene->add_entity(world_entity);

    Ref<Camera> camera = make_ref<Camera>();

    Ref<Entity> player_head = make_ref<Entity>();
    player_head->add_component(make_ref<TransformComponent3D>(glm::vec3(0.0, 0.85, 0.0)));
    player_head->add_component(camera);

    player = make_ref<Entity>();
    player->add_component(make_ref<TransformComponent3D>(Transform3D(glm::vec3(0.0, 18.0, 0.0))));
    player->add_component(make_ref<RigidBody>(AABB(glm::vec3(), glm::vec3(0.9))));
    player->add_component(make_ref<Player>(world));
    player->add_child(player_head);

    scene->add_entity(player);
    scene->set_active_camera(camera);

    // Register blocks
    {
        std::array<std::string, 6> dirt = {"Dirt.png", "Dirt.png", "Dirt.png", "Dirt.png", "Dirt.png", "Dirt.png"};
        BlockRegistry::get().register_block(make_ref<Block>("dirt", dirt));

        std::array<std::string, 6> grass = {"Grass_Side.png", "Grass_Side.png", "Grass_Side.png", "Grass_Side.png", "Grass_Top.png", "Dirt.png"};
        BlockRegistry::get().register_block(make_ref<Block>("grass", grass));

        std::array<std::string, 6> stone = {"Stone.png", "Stone.png", "Stone.png", "Stone.png", "Stone.png", "Stone.png"};
        BlockRegistry::get().register_block(make_ref<Block>("stone", stone));

        std::array<std::string, 6> water = {"Water.png", "Water.png", "Water.png", "Water.png", "Water.png", "Water.png"};
        BlockRegistry::get().register_block(make_ref<Block>("water", water));
    }

    BlockRegistry::get().create_texture_array();
    material->set_param("images", BlockRegistry::get().get_texture_array());

    if (args.has("disable-save"))
    {
        info("World won't be saved, `--disable-save` is present.");
    }

    gen = make_ref<WorldGenerator>(world, args.has("disable-save"));
    gen->set_terrain(make_ref<FlatTerrainGenerator>());

    {
        RenderGraph& graph = RenderGraph::get();

        // depth prepass
        {
            graph.begin_render_pass({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
            graph.end_render_pass();
        }

        // main color pass
        {
            graph.begin_render_pass({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
            graph.end_render_pass();
        }

        RenderingDriver::get()->draw_graph(graph);
        graph.reset();

        EXPECT(RenderingDriver::get()->initialize_imgui());
    }

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
    (void)RenderingDriverVulkan::get()->get_device().waitIdle();

    gen->stop_workers();
    BlockRegistry::get().destroy();
    Font::deinit_library();
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
    ZoneScoped;

    std::optional<SDL_Event> event;

#ifdef __has_debug_menu
    ImGuiIO& imgui_io = ImGui::GetIO();
#endif

    while ((event = window->poll_event()))
    {
        switch (event->type)
        {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            window->close();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
            Result<> result = RenderingDriver::get()->configure_surface(*window, VSync::Off);
            ERR_EXPECT_B(result, "Failed to configure the surface");
        }
        break;
        default:
            break;
        }

#ifdef __has_debug_menu
        ImGui_ImplSDL3_ProcessEvent(&*event);

        if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
        {
            continue;
        }
#endif

        Input::get().process_event(*window, event.value());
    }

    RenderingDriver::get()->poll();

#ifdef __has_debug_menu
    ImGui::NewFrame();

    if (ImGui::Begin("General info"))
    {
        size_t rss = get_current_rss();
        ImGui::Text("CPU Memory: %zu bytes", rss);
    }
    ImGui::End();
#endif

    const glm::vec3 player_pos = player->get_component<TransformComponent3D>()->get_global_transform().position();
    gen->set_player_pos(player_pos);
    gen->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));

    Ref<Scene>& scene = Scene::get_active_scene();

    scene->tick();

    Input::get().post_events();

    RenderGraph& graph = RenderGraph::get();

    // depth prepass
    {
        graph.begin_render_pass({.name = "depth pass", .depth_attachment = RenderPassDepthAttachment{.save = true}});
        scene->encode_draw_calls(graph);
        graph.end_render_pass();
    }

    // main color pass
    {
        graph.begin_render_pass({.name = "main pass", .color_attachments = {RenderPassColorAttachment{.surface_texture = true}}, .depth_attachment = RenderPassDepthAttachment{.load = true}});
        scene->encode_draw_calls(graph);

        graph.add_imgui_draw();
        graph.end_render_pass();
    }

    RenderingDriver::get()->draw_graph(graph);

    // Reset after drawing the graph, not before drawing so that `main` can add copy calls.
    graph.reset();
}

static void register_all_classes()
{
    Entity::register_class();
    Component::register_class();
    VisualComponent::register_class();
    RigidBody::register_class();
    Player::register_class();
    Camera::register_class();
    TransformComponent3D::register_class();

    World::register_class();
    Block::register_class();
    TerrainGenerator::register_class();
    FlatTerrainGenerator::register_class();

    Font::register_class();

    RenderingDriver::register_class();
    Buffer::register_class();
    Texture::register_class();
    Mesh::register_class();
    MaterialLayout::register_class();
    Material::register_class();
    Shader::register_class();

#ifdef __has_vulkan
    RenderingDriverVulkan::register_class();
    BufferVulkan::register_class();
    TextureVulkan::register_class();
    MeshVulkan::register_class();
    MaterialLayoutVulkan::register_class();
    MaterialVulkan::register_class();
#endif

#ifdef __has_webgpu
    RenderingDriverWebGPU::register_class();
    BufferWebGPU::register_class();
    TextureWebGPU::register_class();
    MeshWebGPU::register_class();
    MaterialLayoutWebGPU::register_class();
    MaterialWebGPU::register_class();
#endif
}

#else

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#endif // DOCTEST_CONFIG_DISABLE
