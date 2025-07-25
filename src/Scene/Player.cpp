#include "Scene/Player.hpp"
#include "Input.hpp"
#include "Ray.hpp"
#include "Scene/Components/MeshInstance.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "World/Registry.hpp"

struct CubeHighlightUniforms
{
    glm::mat4x4 model_matrix;
    glm::vec3 color;
};

void Player::start()
{
    m_transform = m_entity->get_component<Transformed3D>();
    m_body = m_entity->get_component<RigidBody>();

    m_camera = m_entity->get_child(0)->get_component<Camera>();

    // Create a special cube mesh and material to highlight targeted blocks.
    auto shader_result = Shader::compile("assets/shaders/cube_highlight.wgsl", {}, {.vertex = true, .fragment = true});
    Ref<Shader> shader = shader_result.value();
#ifdef __platform_web
    shader->set_binding("uniforms", Binding{.kind = BindingKind::UniformBuffer, .shader_stage = ShaderStageKind::Fragment, .group = 0, .binding = 0});
#endif

    Result<Ref<MaterialLayout>> material_layout_result = RenderingDriver::get()->create_material_layout(shader, {.transparency = false, .priority = PriorityBefore}, std::nullopt, CullMode::Back, PolygonMode::Line);
    EXPECT(material_layout_result);
    Ref<MaterialLayout> material_layout = material_layout_result.value();

    Result<Ref<Material>> material_result = RenderingDriver::get()->create_material(material_layout);
    EXPECT(material_result);
    Ref<Material> material = material_result.value();

    Result<Ref<Buffer>> buffer_result = RenderingDriver::get()->create_buffer(sizeof(CubeHighlightUniforms), {.copy_dst = true, .uniform = true});
    ERR_EXPECT_R(buffer_result, "Failed to create buffer for cube highlight");
    m_cube_highlight_buffer = buffer_result.value();

    material->set_param("uniforms", m_cube_highlight_buffer);

    m_cube_highlight = make_ref<Entity>();
    m_cube_highlight->add_component(make_ref<Transformed3D>());
    m_cube_highlight->add_component(make_ref<MeshInstance>(m_cube_mesh, material));
    m_entity->get_scene()->add_entity(m_cube_highlight);
}

void Player::tick(double delta)
{
    ZoneScopedN("Player::tick");

    Transform3D transform = m_transform->get_transform();

    const glm::vec3 forward = transform.forward();
    const glm::vec3 right = transform.right();
    const glm::vec3 up(0.0, 1.0, 0.0);
    const glm::vec3 dir = Input::get().get_movement_vector();

    if (Input::get().is_mouse_grabbed())
    {
        ZoneScopedN("Player::tick.mouse_movements");

        constexpr float mouse_sensibility = 0.03f;

        const glm::vec2 relative = Input::get().get_mouse_relative();
        const glm::quat q_yaw = glm::angleAxis(relative.x * mouse_sensibility, up);

        transform.rotation() *= q_yaw;
        m_transform->set_transform(transform);

        Ref<Transformed3D> camera_transform_comp = m_camera->get_entity()->get_component<Transformed3D>();
        Transform3D camera_transform = camera_transform_comp->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * mouse_sensibility, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        camera_transform_comp->set_transform(camera_transform);
    }

    const glm::vec3 camera_forward = m_camera->get_entity()->get_component<Transformed3D>()->get_global_transform().forward();
    Ray ray(transform.position(), camera_forward);
    float t = 0.0;

    {
        ZoneScopedN("Player::tick.aim");

        while (t < 5.0)
        {
            glm::vec3 pos = ray.at(t);

            int64_t x = (int64_t)(pos.x + 0.5);
            int64_t y = (int64_t)(pos.y + 0.5);
            int64_t z = (int64_t)(pos.z + 0.5);

            BlockState state = m_world->get_block_state(x, y, z);

            if (!state.is_air())
            {
                m_cube_highlight->get_component<MeshInstance>()->set_visible(true);
                on_block_aimed(state, x, y, z, ray.dir());
                break;
            }
            else
            {
                m_cube_highlight->get_component<MeshInstance>()->set_visible(false);
            }

            t += 0.1;
        }
    }
    m_body->velocity() += forward * dir.z * m_speed;
    m_body->velocity() += up * dir.y * m_speed;
    m_body->velocity() += right * dir.x * m_speed;

    if (m_gravity_enabled && !m_body->is_on_ground(m_world))
    {
        m_body->velocity().y -= m_gravity_value * (float)delta;
    }

    m_body->move_and_collide(m_world, delta);
    m_body->velocity().x = 0; // FIXME: Add friction, ...
    m_body->velocity().y = 0;
    m_body->velocity().z = 0;
}

enum class Face
{
    North,
    South,
    West,
    East,
    Top,
    Bottom,
};

static Face get_face(glm::vec3 dir)
{
    std::array<glm::vec3, 6> normals{
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };
    std::array<Face, 6> faces{Face::North, Face::South, Face::West, Face::East, Face::Top, Face::Bottom};

    float max_dot = 0.0;
    Face max_face = Face::North;

    for (size_t i = 0; i < 6; i++)
    {
        glm::vec3 normal = -normals[i];
        Face face = faces[i];

        float dot = glm::dot(dir, normal);

        if (dot >= 0.0 && dot > max_dot)
        {
            max_dot = dot;
            max_face = face;
        }
    }

    return max_face;
}

void Player::on_block_aimed(BlockState state, int64_t x, int64_t y, int64_t z, glm::vec3 dir)
{
    Transform3D transform(glm::vec3((float)x, (float)y, (float)z));
    // FIXME: transform does not matter for MeshInstance, probably should instead of manipulating a buffer directly.
    // m_cube_highlight->get_component<Transformed3D>()->set_transform(transform);

    CubeHighlightUniforms uniforms{};
    uniforms.model_matrix = transform.translation_matrix();
    uniforms.color = glm::vec3(1.0, 1.0, 0.0);

    RenderingDriver::get()->update_buffer(m_cube_highlight_buffer, View(uniforms).as_bytes(), 0);

    if (Input::get().is_action_pressed(Action::Attack) && !state.is_air())
    {
        m_world->set_block_state(x, y, z, BlockState());
    }
    else if (Input::get().is_action_pressed(Action::Place))
    {
        Face face = get_face(dir);

        int64_t x2 = face == Face::West ? x + 1 : (face == Face::East ? x - 1 : x);
        int64_t y2 = face == Face::Top ? y + 1 : (face == Face::Bottom ? y - 1 : y);
        int64_t z2 = face == Face::South ? z + 1 : (face == Face::North ? z - 1 : z);

        m_world->set_block_state(x2, y2, z2, BlockState(BlockRegistry::get().get_block_id("stone")));
    }
}
