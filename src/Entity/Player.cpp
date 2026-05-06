#include "Entity/Player.hpp"

#include "AABB.hpp"
#include "Core/Math.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Network/Packet.hpp"
#include "Profiler.hpp"
#include "Rpc.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <imgui.h>

Player::Player()
    : Mob("player")
{
    m_name = "Player";
}

void Player::on_ready()
{
    m_aabb = AABB(glm::vec3(), glm::vec3(0.35, 0.9, 0.35));

    if (m_local_player)
    {
        m_camera = newobj(Camera);
        m_camera->get_transform().position() = glm::vec3(0, 0.85, 0);
        add_child(m_camera);
        m_world->set_active_camera(m_camera);
    }

    // m_highlight_mesh = create_cube_mesh(glm::vec3(1.01f));
    // m_highlight_model_buffer = EXPECT(Buffer::create(sizeof(ChunkModel), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));

    // m_highlight_shader = EXPECT(Shader::load("assets/shaders/cube_highlight.wgsl"));
    // m_highlight_shader->set_binding("env", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 0, BindingAccess::Read));
    // m_highlight_shader->set_binding("model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 1, BindingAccess::Read));

    // m_highlight_material = EXPECT(Material::create(m_highlight_shader, MaterialFlagBits::Transparency | MaterialFlagBits::Priority, PolygonMode::Fill, WGPUCullMode_Back, UVType::UVT));
    // m_highlight_material->set_param("env", Renderer::get().get_world_environment());
    // m_highlight_material->set_param("model", m_highlight_model_buffer);

    m_model = EXPECT(Model::load("assets/models/player.json"));
    m_animator.set_model(m_model);

    Model::Info info{.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 100.0, 0.0))};
    m_model->get_global_buffer()->update(View(info).as_bytes());

    m_attack_damage = 1;
}

void Player::tick(float delta)
{
    ZoneScopedN("Player::tick");

    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(true);
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(false);
    }

    Transform3D transform = m_transform;

    const glm::vec3 up(0.0, 1.0, 0.0);

    if (Input::is_mouse_grabbed() && m_local_player)
    {
        constexpr float mouse_sensibility = 0.03f;

        const glm::vec2 relative = Input::get_mouse_relative();
        const glm::quat q_yaw = glm::angleAxis(relative.x * mouse_sensibility, up);

        transform.rotation() *= q_yaw;
        m_transform = transform;

        Ref<Camera> camera = m_children.get_unchecked(0).cast_to<Camera>();
        Transform3D camera_transform = camera->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * mouse_sensibility, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        camera->get_transform() = camera_transform;
    }

    if (m_local_player)
    {
        const std::optional<RaycastResult> raycast_result = m_world->raycast(Ray(m_camera->get_global_transform().position(), m_camera->get_global_transform().forward()), 4.0);
        if (Input::is_mouse_grabbed() && raycast_result.has_value())
        {
            int64_t x = raycast_result->x;
            int64_t y = raycast_result->y;
            int64_t z = raycast_result->z;
            m_aimed_block = glm::vec3(x, y, z);

            if (Input::is_action_pressed("attack") && !m_block_broken)
            {
                // m_world->set_block_state(x, y, z, BlockState());
                // m_block_broken = true;
            }
            else if (!Input::is_action_pressed("attack"))
            {
                m_block_broken = false;
            }

            if (Input::is_action_pressed("interact") && !m_block_placed)
            {
                glm::vec3 normal = face_normal(raycast_result->face);

                m_world->set_block_state(int64_t(float(x) + normal.x), int64_t(float(y) + normal.y), int64_t(float(z) + normal.z), BlockState(BlockRegistry::get_block_id("stone")));
                m_block_placed = true;
            }
            else if (!Input::is_action_pressed("interact"))
            {
                m_block_placed = false;
            }
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

            // TODO
            // if (hit)
            // {
            //     hit->entity->call_rpc("on_hit", *this);
            // }
        }
        m_was_attack_pressed = is_attack_pressed;
    }

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");

    float updown_dir = 0.0;
    if (Input::is_mouse_grabbed() && !m_gravity_enabled && m_local_player)
    {
        updown_dir = Input::get_action_value("up") - Input::get_action_value("down");
    }

    if (Input::is_mouse_grabbed() && (glm::length2(dir) != 0.0 || updown_dir != 0.0) && m_local_player)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * m_speed;
        m_velocity += move * delta;
    }

    if (!m_on_ground && m_gravity_enabled)
    {
        m_velocity += glm::vec3(0, -1, 0) * m_gravity_value * delta;
    }

    move_and_collide(m_collision_enabled);

    // Reset velocity after movements.
    m_velocity.x = 0.0;
    m_velocity.y = std::min(m_velocity.x, 40.0f);
    m_velocity.z = 0.0;

    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed() && m_local_player)
    {
        Input::set_mouse_grabbed(true);
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed() && m_local_player)
    {
        Input::set_mouse_grabbed(false);
    }

    if (!m_local_player)
    {
        m_animator.play("walk");
        m_animator.tick(delta);
    }

    if (m_local_player && !Engine::singleton->is_server())
    {
        SendPlayerTransformPacket p{};
        p.id = m_id;
        p.position = get_global_transform().position();
        p.rotation = get_global_transform().rotation();
        Engine::singleton->get_connection().send(Engine::singleton->get_connection().create_packet(p));
    }
}

void Player::draw(const RenderPassNode& node)
{
    if (!m_local_player)
    {
        m_model->encode(node, get_global_transform());
    }
    else
    {
        // if (m_aimed_block.has_value())
        // {
        //     m_highlight_model.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.value()));
        //     m_highlight_model_buffer->update(View(m_highlight_model).as_bytes());

        //     encoder.bind_material(m_highlight_material);
        //     encoder.bind_index_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Index));
        //     encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Position), 0);
        //     encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::UV), 1);
        //     encoder.bind_vertex_buffer(m_highlight_mesh->get_buffer(MeshBufferKind::Normal), 2);
        //     encoder.draw(m_highlight_mesh->vertex_count(), 1);
        // }

        if (node.is_final_pass())
        {
            if (ImGui::Begin("Player"))
            {
                ImGui::Checkbox("Gravity", &m_gravity_enabled);
                ImGui::Checkbox("Collision", &m_collision_enabled);
                ImGui::Text("Position: %f %f %f", m_transform.position().x, m_transform.position().y, m_transform.position().z);
                ImGui::SliderFloat("Speed", &m_speed, 1.0, 70.0);
            }
            ImGui::End();
        }
    }
}

void Player::die()
{
}

void Player::on_hit_by(Entity& entity)
{
    (void)entity;
}
