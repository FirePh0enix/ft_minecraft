#include "MeshPrimitives.hpp"
#include "Render/Driver.hpp"
#include "Render/DriverVulkan.hpp"
#include "Window.hpp"

#include <tracy/Tracy.hpp>

#include <SDL3_image/SDL_image.h>

#include <print>

struct BlockInstanceData
{
    glm::vec3 position;
    glm::vec3 textures0;
    glm::vec3 textures1;
    uint8_t visibility;
    uint8_t gradient;
    uint8_t gradient_type;
    uint8_t pad = 0;
};

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    initialize_error_handling(argv[0]);

    tracy::SetThreadName("Main");

    static const int width = 1280;
    static const int height = 720;

    Window window("ft_vox", width, height);

    RenderingDriver::create_singleton<RenderingDriverVulkan>();

    auto init_result = RenderingDriver::get()->initialize(window);
    EXPECT(init_result);

    auto instance_buffer_result = RenderingDriver::get()->create_buffer(sizeof(BlockInstanceData) * 1, {.copy_dst = true, .vertex = true});
    EXPECT(instance_buffer_result);
    Ref<Buffer> instance_buffer = instance_buffer_result->ptr();

    BlockInstanceData block_instance{
        .position = glm::vec3(0.0, 0.0, -3.0),
        .textures0 = glm::vec3(0.0, 0.0, 0.0),
        .textures1 = glm::vec3(0.0, 0.0, 0.0),
        .visibility = 0xff,
        .gradient = 0,
        .gradient_type = 0,
    };
    Span<BlockInstanceData> span{block_instance};
    instance_buffer->update(span.as_bytes());

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
        InstanceLayoutInput{.type = ShaderType::Vec3, .offset = 0},
        InstanceLayoutInput{.type = ShaderType::Vec3, .offset = sizeof(glm::vec3)},
        InstanceLayoutInput{.type = ShaderType::Vec3, .offset = sizeof(glm::vec3) * 2},
        InstanceLayoutInput{.type = ShaderType::Uint, .offset = sizeof(glm::vec3) * 3},
    };
    InstanceLayout instance_layout(inputs, sizeof(BlockInstanceData));
    auto material_layout_result = RenderingDriverVulkan::get()->create_material_layout(shaders, params, {.transparency = true}, instance_layout, CullMode::None, PolygonMode::Fill, false, false);
    EXPECT(material_layout_result);
    Ref<MaterialLayout> material_layout = material_layout_result.value();

    auto material_result = RenderingDriverVulkan::get()->create_material(material_layout.ptr());
    EXPECT(material_result);
    Ref<Material> material = material_result.value();

    material->set_param("textures", texture_array);

    auto cube_result = create_cube_with_separate_faces(glm::vec3(1.0)); // create_cube_with_separate_faces(glm::vec3(1.0), glm::vec3(-0.5));
    EXPECT(cube_result);
    Ref<Mesh> cube = cube_result.value();

    RenderGraph graph;

    glm::mat4 view_matrix = glm::perspective(glm::radians(70.0), (double)width / (double)height, 0.01, 10'000.0);
    view_matrix[1][1] *= -1;

    while (window.is_running())
    {
        std::optional<SDL_Event> event;

        while ((event = window.poll_event()))
        {
            switch (event->type)
            {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                window.close();
                break;
            default:
                break;
            }
        }

        graph.reset();

        graph.begin_render_pass();
        graph.add_draw(cube.ptr(), material.ptr(), view_matrix, 1, instance_buffer.ptr());
        graph.end_render_pass();

        RenderingDriver::get()->draw_graph(graph);
    }
}
