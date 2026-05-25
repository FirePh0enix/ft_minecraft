#include "Entity/Player.hpp"

#include "AABB.hpp"
#include "Core/Math.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Input.hpp"
#include "Model.hpp"
#include "Network/Packet.hpp"
#include "Render/Renderer.hpp"
#include "World/World.hpp"

#include <imgui.h>

Player::Player()
{
    m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
}

void Player::on_ready()
{
    m_inventory = EXPECT(newref<Inventory>());
    m_inventory->set_quick_stack(0, ItemStack(Engine::get().blocks().get_block_id("stone"), 16));
    m_inventory->set_quick_stack(1, ItemStack(Engine::get().blocks().get_block_id("dirt"), 16));

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
    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed() && !m_open_inventory && m_local_player)
    {
        Input::set_mouse_grabbed(true);
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed() && m_local_player)
    {
        Input::set_mouse_grabbed(false);
    }

    if (Input::is_action_just_pressed("open_inventory"))
    {
        m_open_inventory = !m_open_inventory;
        m_inventory->set_open(m_open_inventory);
        Input::set_mouse_grabbed(!m_open_inventory);
    }

    if (m_local_player)
    {
        if (Input::is_action_just_pressed("1"))
            set_slot(0);
        if (Input::is_action_just_pressed("2"))
            set_slot(1);
        if (Input::is_action_just_pressed("3"))
            set_slot(2);
        if (Input::is_action_just_pressed("4"))
            set_slot(3);
        if (Input::is_action_just_pressed("5"))
            set_slot(4);
        if (Input::is_action_just_pressed("6"))
            set_slot(5);
        if (Input::is_action_just_pressed("7"))
            set_slot(6);
        if (Input::is_action_just_pressed("8"))
            set_slot(7);
        if (Input::is_action_just_pressed("9"))
            set_slot(8);
    }

    AABB item_box = get_aabb().translate(get_position()).grow(glm::vec3(0.5));
    Vector<Ref<Entity>> entities = m_world->get_dimension(World::overworld).cast_box(item_box);
    for (const Ref<Entity>& entity : entities)
    {
        if (Ref<ItemEntity> item = entity.cast_to<ItemEntity>())
        {
            m_inventory->add_stack(ItemStack(item->get_block(), 1));
            m_world->remove_entity(World::overworld, item);
        }
    }

    Transform3D transform = m_transform;

    const glm::vec3 up(0.0, 1.0, 0.0);

    if (are_input_available() && m_local_player)
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

    if (m_local_player && are_input_available())
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

            if (Input::is_action_just_pressed("interact"))
            {
                ItemStack& stack = m_inventory->get_quick_stack(m_slot);
                if (stack.get_block() != 0 && m_world->get_block_state(result.block_pos.x + int64_t(result.normal.x), result.block_pos.y + int64_t(result.normal.y), result.block_pos.z + int64_t(result.normal.z)).is_air())
                {
                    m_world->set_block_state(result.block_pos.x + int64_t(result.normal.x), result.block_pos.y + int64_t(result.normal.y), result.block_pos.z + int64_t(result.normal.z), BlockState(stack.get_block()));
                    stack.set_count(stack.count() - 1);
                }
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
    if (are_input_available() && !has_gravity() && m_local_player)
    {
        updown_dir = Input::get_action_value("jump") - Input::get_action_value("down");
    }

    if (are_input_available() && (glm::length2(dir) != 0.0 || updown_dir != 0.0) && m_local_player)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * m_speed;
        m_velocity += move * delta;
    }

    if (are_input_available() && m_on_ground && Input::is_action_just_pressed("jump"))
    {
        m_velocity += glm::vec3(0, 1, 0) * m_jump_force;
    }

    if (has_gravity())
    {
        m_velocity += glm::vec3(0, -1, 0) * m_gravity_value * delta;
    }

    if (has_gravity())
    {
        move_and_collide();
    }
    else
    {
        get_transform().position() += m_velocity;
    }

    // Reset velocity after movements.
    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (has_gravity())
        m_velocity.y = std::clamp(m_velocity.y, -25.0f, 25.0f);
    else
        m_velocity.y = 0.0;

    if (!m_local_player)
    {
        m_animator.play("walk");
        m_animator.tick(delta);
    }

    if (m_local_player)
    {
        m_inventory->set_selected_slot(m_slot);
        m_inventory->update(delta);
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

    if (m_local_player && m_aimed_block.has_value())
    {
        SimpleUniforms uniforms(glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.value())) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.01f)), glm::vec4(aim_color, 0.4));
        m_aim_buffer->update(View(uniforms).as_bytes());
        Renderer::get().record_simple_shape(node, m_aim_material);
    }
}

void Player::draw_ui(const RenderPassNode& node)
{
    if (m_local_player)
    {
        m_inventory->draw(node);
    }
}

void Player::die()
{
}

void Player::on_hit_by(Entity& entity)
{
    (void)entity;
}
