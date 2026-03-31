#include "Entity/Player.hpp"
#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Core/Definitions.hpp"
#include "Core/Print.hpp"
#include "Entity/Entity.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "Render/Driver.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"

Player::Player()
{
    m_name = "Player";
}

static Ref<Mesh> create_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    std::array<uint16_t, 36> indices{
        0, 1, 2,
        2, 3, 0, // front

        20, 21, 22,
        22, 23, 20, // back

        4, 5, 6,
        6, 7, 4, // right

        12, 13, 14,
        14, 15, 12, // left

        8, 9, 10,
        10, 11, 8, // top

        16, 17, 18,
        18, 19, 16, // bottom
    };
    // clang-format on

    std::array<glm::vec3, 24> vertices{
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // front
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // back
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // left
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // right
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z), // top
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // bottom
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
    };

    std::array<glm::vec2, 24> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    std::array<glm::vec3, 24> normals{
        glm::vec3(0.0, 0.0, 1.0), // front
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),

        glm::vec3(0.0, 0.0, -1.0), // back
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),

        glm::vec3(1.0, 0.0, 0.0), // left
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),

        glm::vec3(-1.0, 0.0, 0.0), // right
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),

        glm::vec3(0.0, 1.0, 0.0), // top
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),

        glm::vec3(0.0, -1.0, 0.0), // bottom
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };

    View<uint16_t> indices_span = indices;

    return Mesh::create_from_data(indices_span.as_bytes(), vertices, normals, View(uvs).as_bytes(), IndexType::Uint16);
}

void Player::on_ready()
{
    m_aabb = AABB(glm::vec3(), glm::vec3(0.35, 0.9, 0.35));

    m_camera = newobj(Camera);
    m_camera->get_transform().position() = glm::vec3(0, 0.85, 0);
    add_child(m_camera);
    m_world->set_active_camera(m_camera);

    m_highlight_mesh = create_cube_mesh();
    m_highlight_model_buffer = RenderingDriver::get()->create_buffer(sizeof(Model), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest).value_or(nullptr);

    m_highlight_shader = Shader::load("assets/shaders/cube_highlight.wgsl").value_or(nullptr);
    m_highlight_shader->set_binding("env", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 0, BindingAccess::Read));
    m_highlight_shader->set_binding("model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 1, BindingAccess::Read));

    m_highlight_material = RenderingDriver::get()->create_material(m_highlight_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back, UVType::UVT, "HIGHLIGHT_CUBE");
    m_highlight_material->set_param("env", m_world->get_env_buffer());
    m_highlight_material->set_param("model", m_highlight_model_buffer);

    m_attack_damage = 1;
}

void Player::tick(float delta)
{
    ZoneScopedN("Player::tick");

    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(true);
        return;
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(false);
        return;
    }

    Transform3D transform = m_transform;

    const glm::vec3 up(0.0, 1.0, 0.0);

    if (Input::is_mouse_grabbed())
    {
        ZoneScopedN("Player::tick.mouse_movements");

        constexpr float mouse_sensibility = 0.03f;

        const glm::vec2 relative = Input::get_mouse_relative();
        const glm::quat q_yaw = glm::angleAxis(relative.x * mouse_sensibility, up);

        transform.rotation() *= q_yaw;
        m_transform = transform;

        Ref<Camera> camera = m_children[0].cast_to<Camera>();
        Transform3D camera_transform = camera->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * mouse_sensibility, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        camera->get_transform() = camera_transform;
    }

    const std::optional<RaycastResult> raycast_result = m_world->raycast(Ray(m_camera->get_global_transform().position(), m_camera->get_global_transform().forward()), 5.0);
    if (raycast_result.has_value())
    {
        m_aimed_block = glm::vec3(raycast_result->x, raycast_result->y, raycast_result->z);
    }
    else
    {
        m_aimed_block = std::nullopt;
    }

    bool is_attack_pressed = Input::is_action_pressed("attack");

    if (is_attack_pressed && !m_was_attack_pressed)
    {
        Ray ray(
            m_camera->get_global_transform().position(),
            m_camera->get_global_transform().forward());

        float attack_range = 5.0f;
        // Max range for entity raycast is either attack range, or either distance of first block detected.
        float max_dist = raycast_result.has_value() ? raycast_result->distance : attack_range;

        auto hit = m_world->raycast_entities(ray, max_dist, this);

        if (hit)
        {
            hit->entity->call_rpc("on_hit", *this);
        }
    }

    m_was_attack_pressed = is_attack_pressed;

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");

    float updown_dir = 0.0;
    if (!m_gravity_enabled)
    {
        updown_dir = Input::get_action_value("up") - Input::get_action_value("down");
    }

    if (glm::length2(dir) != 0.0 || updown_dir != 0.0)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * m_speed;
        m_velocity += move * delta;
    }

    if (!m_on_ground && m_gravity_enabled)
    {
        m_velocity += glm::vec3(0, -1, 0) * m_gravity_value * delta;
    }

    move_and_collide(true);

    // Reset velocity after movements.
    m_velocity = glm::vec3(0);
}

void Player::draw(RenderPassEncoder& encoder)
{
    if (m_aimed_block.has_value())
    {
        // (void)encoder;
        m_highlight_model.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.value()));
        m_highlight_model_buffer->update(View(m_highlight_model).as_bytes());

        encoder.bind_material(m_highlight_material);
        encoder.bind_index_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Index));
        encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Position), 0);
        encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::UV), 1);
        encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Normal), 2);
        encoder.draw(m_highlight_mesh->vertex_count(), 1);
    }

    if (ImGui::Begin("Player"))
    {
        ImGui::Checkbox("Gravity", &m_gravity_enabled);
        ImGui::Text("Position: %f %f %f", m_transform.position().x, m_transform.position().y, m_transform.position().z);
    }
    ImGui::End();
}

// enum class Face
// {
//     North,
//     South,
//     West,
//     East,
//     Top,
//     Bottom,
// };

// static Face get_face(glm::vec3 dir)
// {
//     std::array<glm::vec3, 6> normals{
//         glm::vec3(0.0, 0.0, -1.0),
//         glm::vec3(0.0, 0.0, 1.0),
//         glm::vec3(1.0, 0.0, 0.0),
//         glm::vec3(-1.0, 0.0, 0.0),
//         glm::vec3(0.0, 1.0, 0.0),
//         glm::vec3(0.0, -1.0, 0.0),
//     };
//     std::array<Face, 6> faces{Face::North, Face::South, Face::West, Face::East, Face::Top, Face::Bottom};

//     float max_dot = 0.0;
//     Face max_face = Face::North;

//     for (size_t i = 0; i < 6; i++)
//     {
//         glm::vec3 normal = -normals[i];
//         Face face = faces[i];

//         float dot = glm::dot(dir, normal);

//         if (dot >= 0.0 && dot > max_dot)
//         {
//             max_dot = dot;
//             max_face = face;
//         }
//     }

//     return max_face;
// }

// void Player::on_block_aimed(BlockState state, int64_t x, int64_t y, int64_t z, glm::vec3 dir)
// {
//     Transform3D transform(glm::vec3((float)x, (float)y, (float)z));
//     // FIXME: transform does not matter for MeshInstance, probably should instead of manipulating a buffer directly.
//     // m_cube_highlight->get_transform->set_transform(transform);

//     // CubeHighlightUniforms uniforms{};
//     // uniforms.model_matrix = transform.translation_matrix();
//     // uniforms.color = glm::vec3(1.0, 1.0, 0.0);

//     // RenderingDriver::get()->update_buffer(m_cube_highlight_buffer, View(uniforms).as_bytes(), 0);

//     if (Input::is_action_pressed("attack") && !state.is_air())
//     {
//         m_world->set_block_state(x, y, z, BlockState());
//     }
//     else if (Input::is_action_pressed("place"))
//     {
//         Face face = get_face(dir);

//         int64_t x2 = face == Face::West ? x + 1 : (face == Face::East ? x - 1 : x);
//         int64_t y2 = face == Face::Top ? y + 1 : (face == Face::Bottom ? y - 1 : y);
//         int64_t z2 = face == Face::South ? z + 1 : (face == Face::North ? z - 1 : z);

//         m_world->set_block_state(x2, y2, z2, BlockState(BlockRegistry::get_block_id("stone")));
//     }
// }

void Player::die() {}

void Player::on_hit_by(Entity& entity) {}