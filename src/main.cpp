#include "Camera.hpp"
#include "Font.hpp"
#include "Input.hpp"
#include "MeshPrimitives.hpp"
#include "Render/Driver.hpp"
#include "Render/DriverVulkan.hpp"
#include "Scene/Scene.hpp"
#include "Window.hpp"
#include "World/World.hpp"

#include <SDL3_image/SDL_image.h>

#include <print>

static void register_all_classes();

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    initialize_error_handling(argv[0]);

    tracy::SetThreadName("Main");

    register_all_classes();

    static const int width = 1280;
    static const int height = 720;

    Window window("ft_vox", width, height);

    RenderingDriver::create_singleton<RenderingDriverVulkan>();

    auto init_result = RenderingDriver::get()->initialize(window);
    EXPECT(init_result);

    Ref<Camera> camera = make_ref<Camera>(glm::vec3(10.0, 13.0, 10.0), glm::vec3(), 0.05);

    auto texture_array_result = RenderingDriver::get()->create_texture_array(16, 16, TextureFormat::RGBA8Srgb, {.copy_dst = true, .sampled = true}, 1);
    EXPECT(texture_array_result);
    Ref<Texture> texture_array = texture_array_result.value();

    {
        SDL_IOStream *texture_stream = SDL_IOFromFile("../assets/textures/Dirt.png", "r");
        ERR_COND(texture_stream == nullptr, "cannot open texture");

        SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
        ERR_COND(texture_stream == nullptr, "cannot load texture");

        texture_array->transition_layout(TextureLayout::CopyDst);
        texture_array->update(Span((uint8_t *)texture_surface->pixels, texture_surface->w * texture_surface->h * 4), 0);
        texture_array->transition_layout(TextureLayout::ShaderReadOnly);

        SDL_DestroySurface(texture_surface);
        SDL_CloseIO(texture_stream);
    }

    std::array<ShaderRef, 2> shaders{ShaderRef("assets/shaders/voxel.vert.spv", ShaderKind::Vertex), ShaderRef("assets/shaders/voxel.frag.spv", ShaderKind::Fragment)};
    std::array<MaterialParam, 1> params{MaterialParam::image(ShaderKind::Fragment, "textures", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest})};
    std::array<InstanceLayoutInput, 4> inputs{
        InstanceLayoutInput(ShaderType::Vec3, 0),
        InstanceLayoutInput(ShaderType::Vec3, sizeof(glm::vec3)),
        InstanceLayoutInput(ShaderType::Vec3, sizeof(glm::vec3) * 2),
        InstanceLayoutInput(ShaderType::Uint, sizeof(glm::vec3) * 3),
    };
    InstanceLayout instance_layout(inputs, sizeof(BlockInstanceData));
    auto material_layout_result = RenderingDriverVulkan::get()->create_material_layout(shaders, params, {.transparency = true}, instance_layout, CullMode::Back, PolygonMode::Fill, false, false);
    EXPECT(material_layout_result);
    Ref<MaterialLayout> material_layout = material_layout_result.value();

    auto material_result = RenderingDriverVulkan::get()->create_material(material_layout.ptr());
    EXPECT(material_result);
    Ref<Material> material = material_result.value();

    material->set_param("textures", texture_array);

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0)); // create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    Font::init_library();

    auto font_result = Font::create("../assets/fonts/Anonymous.ttf", 12);
    EXPECT(font_result);
    Ref<Font> font = font_result.value();

    Text text("Hello world", font);
    text.set_scale(glm::vec2(0.01, 0.01));
    text.set_color(glm::vec4(1.0, 1.0, 1.0, 1.0));

    BlockState dirt(1);

    Ref<World> world = make_ref<World>(cube, material);
    world->set_render_distance(10);
    world->generate_flat(dirt);

    Ref<Entity> world_entity = make_ref<Entity>();
    world_entity->add_component(world);

    Scene scene;
    scene.set_active_camera(camera);
    scene.add_entity(world_entity);

    RenderGraph graph;

    while (window.is_running())
    {
        FrameMark;
        ZoneScopedN("main_loop_tick");

        std::optional<SDL_Event> event;

        while ((event = window.poll_event()))
        {
            switch (event->type)
            {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                window.close();
                break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                const float x_rel = event->motion.xrel;
                const float y_rel = event->motion.yrel;

                if (Input::get().is_mouse_grabbed())
                    camera->rotate(x_rel, y_rel);

                break;
            }
            default:
                break;
            }

            Input::get().process_event(window, event.value());
        }

        camera->tick();

        graph.reset();

        graph.begin_render_pass();
        scene.encode_draw_calls(graph);
        text.encode_draw_calls(graph);
        graph.end_render_pass();

        RenderingDriver::get()->draw_graph(graph);
    }
}

static void register_all_classes()
{
    Object::register_class();

    Component::register_class();
    VisualComponent::register_class();

    RenderingDriver::register_class();
    Buffer::register_class();
    Texture::register_class();
    Mesh::register_class();
    MaterialLayout::register_class();
    Material::register_class();

#ifdef __USE_VULKAN__
    RenderingDriverVulkan::register_class();
    BufferVulkan::register_class();
    TextureVulkan::register_class();
    MeshVulkan::register_class();
    MaterialLayoutVulkan::register_class();
    MaterialVulkan::register_class();
#endif
}
