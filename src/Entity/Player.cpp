#include "Entity/Player.hpp"

#include "AABB.hpp"
#include "Block/Inventory.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Math.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Input.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"
#include "Model.hpp"
#include "Network/Packet.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <imgui.h>

struct GPU_ATTRIBUTE ItemBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

Player::Player()
{
    m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
}

void Player::on_ready()
{
    m_chat = EXPECT(newref<Chat>());

    m_inventory_container = EXPECT(newref<InventoryContainer>());
    EXPECT(m_inventory_container->add_layer(27)); // main inventory
    EXPECT(m_inventory_container->add_layer(9));  // toolbar

    m_inventory_container->set_stack(1, 0, ItemStack(Items::crafting_table_block, 16));
    m_inventory_container->set_stack(1, 1, ItemStack(Items::water_bucket, 1));
    m_inventory_container->set_stack(1, 2, ItemStack(Items::stone_block, 16));
    m_inventory_container->set_stack(1, 3, ItemStack(Items::dirt_block, 16));

    m_inventory = EXPECT(newref<PlayerInventory>(m_inventory_container));

    m_model_buffer = EXPECT(Buffer::create(sizeof(ItemBlockModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
    m_material = EXPECT(Material::create(Renderer::get().get_item_block_shader(), MaterialFlagBits::None, WGPUCullMode_Back, UVType::UV));
    m_material->set_param("env", Renderer::get().get_world_environment());
    m_material->set_param("model", m_model_buffer);
    m_material->set_param("images", Engine::get().registry().get_texture_array());

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

        m_breaks_textures[0] = EXPECT(Texture::load("assets/textures/breaks/0.png"));
        m_breaks_textures[1] = EXPECT(Texture::load("assets/textures/breaks/1.png"));
        m_breaks_textures[2] = EXPECT(Texture::load("assets/textures/breaks/2.png"));
        m_breaks_textures[3] = EXPECT(Texture::load("assets/textures/breaks/3.png"));
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
    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_open_chat)
    {
        Input::set_mouse_grabbed(true);
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_open_chat)
    {
        Input::set_mouse_grabbed(false);
    }
    else if (Input::is_action_pressed("escape") && m_local_player && m_opened_inventory.has_value())
    {
        close_inventory();
    }
    else if (Input::is_action_pressed("escape") && m_local_player && m_open_chat)
    {
        m_open_chat = false;
        Input::set_mouse_grabbed(true);
    }

    if (Input::is_action_just_pressed("toggle_chat") && m_local_player && !m_open_chat)
    {
        m_open_chat = true;
        Input::set_mouse_grabbed(false);
    }

    if (Input::is_action_just_pressed("open_inventory") && !m_open_chat)
    {
        if (!m_opened_inventory.has_value())
            open_inventory(m_inventory);
        else
            close_inventory();
        Input::set_mouse_grabbed(!m_opened_inventory.has_value());
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

        if (Input::get_action_value("toolbar_wheel") > 0)
        {
            set_slot((m_slot + 1) % 9);
        }
        else if (Input::get_action_value("toolbar_wheel") < 0)
        {
            if (m_slot == 0)
                set_slot(8);
            else
                set_slot(m_slot - 1);
        }
    }

    AABB item_box = get_aabb().translate(get_position()).grow(glm::vec3(0.5));
    Vector<Ref<Entity>> entities = m_world->get_dimension(World::overworld).cast_box(item_box);
    for (const Ref<Entity>& entity : entities)
    {
        if (Ref<ItemEntity> item = entity.cast_to<ItemEntity>())
        {
            m_inventory->add_stack(ItemStack(item->item(), 1));
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
                m_aimed_block = None;

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
                BlockState state = m_world->get_block_state(result.block_pos.x, result.block_pos.y, result.block_pos.z);
                Ref<Block> block = Engine::get().registry().get_block(state.id);

                if (Ref<InventoryBlock> ib = block.cast_to<InventoryBlock>())
                {
                    ib->open_inventory(result.block_pos, this);
                }
                else
                {
                    ItemStack stack = m_inventory_container->get_stack(1, m_slot);
                    if (stack.item().valid())
                    {
                        Ref<Item> item = Engine::get().registry().get_item(stack.item());
                        item->interact(*m_world, m_dimension, stack, result.block_pos, result.normal);
                        m_inventory_container->set_stack(1, m_slot, stack);
                    }
                }
            }
        }
        else
        {
            m_aimed_block = None;
        }
    }

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");
    const bool in_water = is_in_water();
    const bool chunk_loaded = chunk_is_loaded();

    float updown_dir = 0.0;
    if (are_input_available() && m_local_player && (!has_gravity() || in_water))
    {
        updown_dir = Input::get_axis("down", "jump");
    }

    float movement_damp = in_water ? 1.0f : 1.0f;
    float vertical_movement_damp = in_water ? 1.0f : 1.0f;

    if (are_input_available() && (glm::length2(dir) != 0.0 || updown_dir != 0.0) && m_local_player && chunk_loaded)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * glm::vec3(movement_damp, vertical_movement_damp, movement_damp) * m_speed;
        m_velocity += move * delta;
    }

    if (are_input_available() && m_on_ground && Input::is_action_just_pressed("jump") && !in_water && chunk_loaded)
    {
        m_velocity += glm::vec3(0, 1, 0) * m_jump_force;
    }
    else if (are_input_available() && Input::is_action_pressed("jump") && !in_water && m_previous_frame_in_water && chunk_loaded)
    {
        m_velocity += glm::vec3(0, 1, 0) * m_jump_force;
    }

    if (has_gravity() && chunk_loaded)
    {
        float value = in_water ? 3.7f : 1.0f;
        m_velocity += glm::vec3(0, -1, 0) * m_gravity_value * delta * value;
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

    if (has_gravity() && !in_water)
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

        if (m_opened_inventory.has_value())
            m_opened_inventory.get()->update(delta);
        else
            m_inventory->update(delta);

        if (m_open_chat)
            m_chat->update(delta);
    }

    m_previous_frame_in_water = in_water;

    // if (m_local_player && Engine::get().is_online() && !Engine::get().is_server())
    // {
    //     SendPlayerTransformPacket p{};
    //     p.id = m_id;
    //     p.position = get_global_transform().position();
    //     p.rotation = get_global_transform().rotation();
    //     Engine::get().connection().send(Engine::get().connection().create_packet(p));
    // }
}

void Player::draw(const RenderPassNode& node)
{
    if (!m_local_player)
    {
        m_model->encode(node, get_global_transform());
    }

    if (m_local_player && m_aimed_block.has_value())
    {
        SimpleUniforms uniforms(glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.get())) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.01f)), glm::vec4(aim_color, 0.4));
        m_aim_buffer->update(View(uniforms).as_bytes());
        Renderer::get().record_simple_shape(node, m_aim_material);
    }

    if (m_local_player && m_inventory_container->get_stack(1, m_inventory->selected_slot()).item().valid())
    {
        Id<Item> id = m_inventory_container->get_stack(1, m_inventory->selected_slot()).item();
        Ref<Item> item = Engine::get().registry().get_item(id);
        if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
        {
            Ref<Block> block = Engine::get().registry().block_from_item(m_inventory_container->get_stack(1, m_inventory->selected_slot()).item());

            Transform3D transform = m_camera->get_global_transform();
            transform.scale() = glm::vec3(0.2);
            transform.position() += m_camera->get_global_transform().forward() * 0.5f + m_camera->get_global_transform().right() * 0.35f + m_camera->get_global_transform().up() * -0.3f;
            transform.set_euler_angles(glm::vec3(0, -m_transform.get_euler_angles().y, 0));

            ItemBlockModel matrix(
                transform.to_matrix(),
                glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16)));

            m_model_buffer->update(View(matrix).as_bytes());
            Renderer::get().record_simple_shape(node, m_material);
        }
        else
        {
            // TODO: 3d model for other sprites.
        }
    }
}

void Player::draw_ui(const RenderPassNode& node)
{
    if (m_local_player)
    {
        if (m_opened_inventory.has_value())
            m_opened_inventory.get()->draw(node);
        else
            m_inventory->draw_toolbar(node);

        if (m_open_chat)
            m_chat->draw(node);
    }
}

void Player::process_event(Event& event)
{
    if (m_open_chat)
        m_chat->process_event(event);
}

void Player::save(EntitySerializer& ser) const
{
    LocalVector<ItemStack> stacks;
    EXPECT(stacks.resize(27 + 9));

    const InventoryContainer::Layer& layer = m_inventory_container->get_layer(0);
    for (size_t i = 0; i < 27; i++)
        stacks.get_unchecked(i) = layer.stacks.get_unchecked(i);

    const InventoryContainer::Layer& toolbar_layer = m_inventory_container->get_layer(1);
    for (size_t i = 0; i < 9; i++)
        stacks.get_unchecked(i + 27) = toolbar_layer.stacks.get_unchecked(i);

    Variant array = View(stacks);
    EXPECT(ser.set("inventory_data", array));
}

void Player::load(const EntitySerializer& deser)
{
    Vector<ItemStack> stacks = deser.get_array<ItemStack>("inventory_data").get();

    InventoryContainer::Layer& layer = m_inventory_container->get_layer(0);
    for (size_t i = 0; i < 27; i++)
        layer.stacks.get_unchecked(i) = stacks.get_unchecked(i);

    InventoryContainer::Layer& toolbar_layer = m_inventory_container->get_layer(1);
    for (size_t i = 0; i < 9; i++)
        toolbar_layer.stacks.get_unchecked(i) = stacks.get_unchecked(i + 27);
}

void Player::die()
{
}

void Player::on_hit_by(Entity& entity)
{
    (void)entity;
}

void Player::open_inventory(Ref<Inventory> inventory)
{
    m_opened_inventory = inventory;
    Input::set_mouse_grabbed(false);
}

void Player::close_inventory()
{
    m_opened_inventory.get()->grab_cancel();
    m_opened_inventory = None;
    Input::set_mouse_grabbed(true);
}
