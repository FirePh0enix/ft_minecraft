#include "Entity/Player.hpp"

#include "AABB.hpp"
#include "Core/Math.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Network/Packet.hpp"
#include "Profiler.hpp"
#include "Render/Renderer.hpp"
#include "World/World.hpp"

#include <imgui.h>

Player::Player()
{
    m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
}

void Player::on_ready()
{
    if (m_local_player)
    {
        m_camera = EXPECT(newref<Camera>());
        m_camera->get_transform().position() = glm::vec3(0, 0.85, 0);
        add_child(m_camera);
        m_world->set_active_camera(m_camera);

        m_aim_buffer = EXPECT(Buffer::create(sizeof(SimpleUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
        m_aim_material = EXPECT(Material::create(Renderer::get().get_simple_shader(), MaterialFlagBits::Transparency | MaterialFlagBits::Priority, WGPUCullMode_Back, UVType::UV));
        m_aim_material->set_param("env", Renderer::get().get_world_environment());
        m_aim_material->set_param("model", m_aim_buffer);
    }
    else
    {
        m_model = EXPECT(Model::load("assets/models/player.json"));
        m_animator.set_model(m_model);

        Model::Info info{.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 100.0, 0.0))};
        m_model->get_global_buffer()->update(View(info).as_bytes());
    }

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
        RaycastResult result;
        if (m_world->raycast(Ray(m_camera->get_global_transform().position(), m_camera->get_global_transform().forward()), 4.0f, result))
        {
            if (!result.hit_entity)
                m_aimed_block = glm::vec3(result.block_pos);
            else
                m_aimed_block = std::nullopt;

            if (Input::is_action_just_pressed("attack"))
            {
                if (result.hit_entity)
                {
                    if (Ref<Mob> mob = result.entity)
                        mob->on_hit_by(*this);
                }
            }

            if (m_gamemode == GameMode::Creative && !result.entity && Input::is_action_just_pressed("attack"))
            {
                m_world->set_block_state(result.block_pos.x, result.block_pos.y, result.block_pos.z, BlockState());
            }
            else if (m_gamemode == GameMode::Survival && !result.entity)
            {
                if (Input::is_action_pressed("attack"))
                {
                    if (!m_is_destroing)
                    {
                        m_is_destroing = true;
                        m_destroy_block_pos = result.block_pos;
                    }
                    else if (m_destroy_block_pos != result.block_pos)
                    {
                        m_is_destroing = false;
                        m_destroy_ticks = 0;
                    }

                    m_destroy_ticks += 1;
                    if (m_destroy_ticks >= max_destroy_ticks)
                    {
                        m_world->break_block(result.block_pos.x, result.block_pos.y, result.block_pos.z);
                        m_is_destroing = false;
                        m_destroy_ticks = 0;
                    }
                }
            }
            else
            {
                m_destroy_ticks = 0;
                m_is_destroing = false;
            }
        }
        else
        {
            m_aimed_block = std::nullopt;
        }
    }

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");

    float updown_dir = 0.0;
    if (Input::is_mouse_grabbed() && !has_gravity() && m_local_player)
    {
        updown_dir = Input::get_action_value("jump") - Input::get_action_value("down");
    }

    if (Input::is_mouse_grabbed() && (glm::length2(dir) != 0.0 || updown_dir != 0.0) && m_local_player)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * m_speed;
        m_velocity += move * delta;
    }

    if (Input::is_mouse_grabbed() && m_on_ground && Input::is_action_just_pressed("jump"))
    {
        m_velocity += glm::vec3(0, 1, 0) * m_jump_force;
    }

    if (has_gravity())
    {
        m_velocity += glm::vec3(0, -1, 0) * m_gravity_value * delta;
    }

    move_and_collide();

    // Reset velocity after movements.
    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (has_gravity())
        m_velocity.y = std::clamp(m_velocity.y, -25.0f, 25.0f);
    else
        m_velocity.y = 0.0;

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

    if (m_local_player && Engine::get().is_online() && !Engine::get().is_server())
    {
        SendPlayerTransformPacket p{};
        p.id = m_id;
        p.position = get_global_transform().position();
        p.rotation = get_global_transform().rotation();
        Engine::get().connection().send(Engine::get().connection().create_packet(p));
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
        if (m_aimed_block.has_value())
        {
            SimpleUniforms uniforms(glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.value())) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.01f)), glm::vec4(aim_color, 0.4));
            m_aim_buffer->update(View(uniforms).as_bytes());
            Renderer::get().record_simple_shape(node, m_aim_material);
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
