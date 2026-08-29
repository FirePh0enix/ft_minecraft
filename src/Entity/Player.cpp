#include "Entity/Player.hpp"

#include "AABB.hpp"
#include "Block/Inventory.hpp"
#include "Core/Math.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Entity/LivingEntity.hpp"
#include "Input.hpp"
#include "Inventory/Inventory.hpp"
#include "Item/ItemStack.hpp"
#include "Model.hpp"
#include "Render/Renderer.hpp"
#include "UI/TextInput.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <imgui.h>

#include <memory>

void BetterConsole::process_command(Player *player, std::string_view str)
{
    const struct
    {
        std::string_view name;
        void (BetterConsole::*fn)(Player *, const std::vector<std::string>& args);
    } commands[]{
        {.name = "dim", .fn = &BetterConsole::chgdim},
        {.name = "tp", .fn = &BetterConsole::tp},
        {.name = "give", .fn = &BetterConsole::give},
        {.name = "gamemode", .fn = &BetterConsole::gamemode},
    };

    std::vector<std::string> args;

    std::stringstream ss(str.data());
    std::string arg;
    while (std::getline(ss, arg, ' '))
        args.push_back(arg);

    if (args.size() == 0)
    {
        error("unknown command ``");
        return;
    }

    for (const auto& command : commands)
    {
        if (command.name == args[0])
        {
            auto fn = command.fn;
            (this->*fn)(player, args);
            return;
        }
    }

    error("unknown command `{}`", args[0]);
}

void BetterConsole::tp(Player *player, const std::vector<std::string>& args)
{
    if (args.size() != 4)
    {
        println("usage `/tp <x> <y> <z>`");
        return;
    }

    int64_t x = std::stol(args[1]);
    int64_t y = std::stol(args[2]);
    int64_t z = std::stol(args[3]);

    player->set_position(glm::vec3(x, y, z));
}

void BetterConsole::chgdim(Player *player, const std::vector<std::string>& args)
{
    (void)player;

    if (args.size() != 2)
    {
        println("usage `/dim <dimension>`");
        return;
    }

    if (args[1] == "overworld" || args[1] == "0")
    {
        if (player->get_dimension() != World::overworld)
        {
            player->get_world()->change_dimension(player->id(), World::overworld);
            println("switched to `overworld`");
        }
        else
            println("already in the `overworld` dimension");
    }
    else if (args[1] == "underworld" || args[1] == "1")
    {
        if (player->get_dimension() != World::underworld)
        {
            player->get_world()->change_dimension(player->id(), World::underworld);
            println("switched to `underworld`");
        }
        else
            println("already in the `underworld` dimension");
    }
    else
    {
        println("Unknown dimension `{}`", args[1]);
    }
}

void BetterConsole::give(Player *player, const std::vector<std::string>& args)
{
    if (args.size() == 2)
    {
        Id<Item> item = Engine::get().registry().item_from_name(args[1]);
        if (!item.valid())
        {
            println("invalid item `{}`", args[1]);
            return;
        }

        ItemStack stack(item, 1);
        player->get_inventory()->add_stack(stack);
    }
    else if (args.size() == 3)
    {
        Id<Item> item = Engine::get().registry().item_from_name(args[1]);
        if (!item.valid())
        {
            println("invalid item `{}`", args[1]);
            return;
        }

        ItemStack stack(item, std::stol(args[2]));
        player->get_inventory()->add_stack(stack);
    }
    else
    {
        println("usage `/give <item> [count]`");
        return;
    }
}

void BetterConsole::gamemode(Player *player, const std::vector<std::string>& args)
{
    if (args.size() != 2)
    {
        println("usage `/gamemode <survival|creative|0|1>`");
        return;
    }

    if (args[1] == "survival" || args[1] == "0")
    {
        player->set_gamemode(GameMode::Survival);
    }
    else if (args[1] == "creative" || args[1] == "1")
    {
        player->set_gamemode(GameMode::Creative);
    }
}

struct GPU_ATTRIBUTE ItemBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

struct GPU_ATTRIBUTE ItemModel
{
    glm::mat4 model_matrix;
};

void Player::bind_methods()
{
    type.add_method("break_block", &Player::break_block);
    expose_rpc<Player>("break_block", RpcTarget::Both);

    type.add_method("place_block", &Player::place_block);
    expose_rpc<Player>("place_block", RpcTarget::Both);
}

void Player::on_ready()
{
    m_inventory_container = std::make_shared<InventoryContainer>();
    m_inventory_container->add_layer(27); // main inventory
    m_inventory_container->add_layer(9);  // toolbar
    m_inventory_container->add_layer(4);  // Crafting Ingredients
    m_inventory_container->add_layer(1);  // Crafting Result

    m_model_buffer = EXPECT(Buffer::create(sizeof(ItemBlockModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
    m_hand_item_bg = BindGroup::create(Renderer::get().get_fw_item_block_shader());
    m_hand_item_bg->set_param("camera", Renderer::get().get_fw_camera());
    m_hand_item_bg->set_param("model", m_model_buffer);
    m_hand_item_bg->set_param("images", EXPECT(Engine::get().registry().get_texture_array()->get_view(WGPUTextureViewDimension_2DArray)));

    if (m_local_player)
    {
        m_inventory = std::make_shared<PlayerInventory>(m_inventory_container);

        m_camera = std::make_shared<Camera>();
        m_camera->get_transform().position() = glm::vec3(0, 0.80, 0);
        add_child(m_camera);
        // m_world->set_active_camera(m_camera);
        m_world->set_player(this);

        m_chat = std::make_shared<Widget>();
        m_chat->set_expand_horizontal(true);
        m_chat->set_expand_vertical(true);
        m_chat->set_alignment(ContainerAlignment::Left | ContainerAlignment::Bottom);

        std::shared_ptr<ColorRectWidget> color_rect = std::make_shared<ColorRectWidget>();
        color_rect->set_color(Colors::red);
        color_rect->set_alignment(ContainerAlignment::CenterY);
        m_chat->add_child(color_rect);

        std::shared_ptr<TextInput> chat_input = std::make_shared<TextInput>(Engine::get().get_font());
        chat_input->set_size(Point(Size::percent(45), Size::px(40)));
        chat_input->done_callback().connect(std::bind_front(&Player::on_text_message, this));
        color_rect->add_child(chat_input);

        // m_aim_buffer = EXPECT(Buffer::create(sizeof(SimpleUniforms), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
        // m_aim_material = EXPECT(Material::create(Renderer::get().get_simple_shader(), MaterialFlagBits::Transparency | MaterialFlagBits::Priority, WGPUCullMode_Back, UVType::UV));
        // m_aim_material->set_param("env", Renderer::get().get_world_environment());
        // m_aim_material->set_param("model", m_aim_buffer);

        // m_breaks_textures[0] = EXPECT(Texture::load("assets/textures/breaks/0.png"));
        // m_breaks_textures[1] = EXPECT(Texture::load("assets/textures/breaks/1.png"));
        // m_breaks_textures[2] = EXPECT(Texture::load("assets/textures/breaks/2.png"));
        // m_breaks_textures[3] = EXPECT(Texture::load("assets/textures/breaks/3.png"));
    }
    else
    {
        m_model = EXPECT(Model::load("assets/models/player.json"));
        m_animator.set_model(m_model);

        // Model::Info info{.model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 100.0, 0.0))};
        // m_model->get_global_buffer()->update(View(info).as_bytes());
    }
}

void Player::on_text_message(TextInput& input, std::string_view message)
{
    std::string msg(message);

    input.clear();

    if (msg.starts_with("/"))
    {
        m_console.process_command(this, msg.substr(1));
    }
    else
    {
        println("message is `{}`", message);
    }
}

void Player::tick(float delta)
{
    Entity::tick(delta);
    // println("{} {} {} {} {}", Input::is_action_pressed("attack"), Input::is_mouse_grabbed(), m_opened_inventory.has_value(), m_local_player, m_chat_opened);

    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_chat_opened)
    {
        Input::set_mouse_grabbed(true);
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_chat_opened)
    {
        Input::set_mouse_grabbed(false);
    }
    else if (Input::is_action_pressed("escape") && m_local_player && m_opened_inventory.has_value())
    {
        close_inventory();
    }
    else if (Input::is_action_pressed("escape") && m_local_player && m_chat_opened)
    {
        m_chat_opened = false;
        Input::set_mouse_grabbed(true);
    }

    if (Input::is_action_just_pressed("open_inventory") && m_local_player && !m_chat_opened)
    {
        if (!m_opened_inventory.has_value())
            open_inventory(m_inventory);
        else
            close_inventory();
        Input::set_mouse_grabbed(!m_opened_inventory.has_value());
    }

    if (Input::is_action_just_pressed("toggle_chat") && !m_chat_opened)
    {
        m_chat_opened = true;
        Input::set_mouse_grabbed(false);
    }

    if (m_local_player && !m_chat_opened)
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
    std::vector<std::shared_ptr<Entity>> entities = m_world->get_dimension(World::overworld).cast_box(item_box);
    for (const std::shared_ptr<Entity>& entity : entities)
    {
        if (std::shared_ptr<ItemEntity> item = std::dynamic_pointer_cast<ItemEntity>(entity))
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

        Transform3D camera_transform = m_camera->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * mouse_sensibility, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        m_camera->get_transform() = camera_transform;
    }

    if (m_local_player && are_input_available())
    {
        RaycastResult result;
        if (m_world->raycast(m_dimension, Ray(m_camera->get_global_transform().position(), m_camera->get_global_transform().forward()), 4.0f, result, this))
        {
            if (!result.hit_entity)
                m_aimed_block = glm::vec3(result.block_pos);
            else
                m_aimed_block = std::nullopt;

            m_world->dd().draw_cube(glm::dvec3(result.block_pos) + result.normal, glm::vec3(1.0), Colors::yellow, 0.05f);

            if (Input::is_action_just_pressed("attack") && result.hit_entity)
            {
                if (auto mob = std::dynamic_pointer_cast<LivingEntity>(result.entity))
                    mob->damage(1, id()); // TODO: different tool deals different damages.
            }
            else if (m_gamemode == GameMode::Creative && !result.hit_entity && Input::is_action_just_pressed("attack"))
            {
                call_rpc("break_block", result.block_pos.x, result.block_pos.y, result.block_pos.z);
            }
            else if (m_gamemode == GameMode::Survival && !result.hit_entity)
            {
                if (Input::is_action_pressed("attack"))
                {
                    if (!m_is_destroying)
                    {
                        m_is_destroying = true;
                        m_destroy_block_pos = result.block_pos;
                    }
                    else if (m_destroy_block_pos != result.block_pos)
                    {
                        m_is_destroying = false;
                        m_destroy_ticks = 0;
                    }

                    m_destroy_ticks += 1;
                    if (m_destroy_ticks >= max_destroy_ticks)
                    {
                        call_rpc("break_block", result.block_pos.x, result.block_pos.y, result.block_pos.z);
                        m_is_destroying = false;
                        m_destroy_ticks = 0;
                    }
                }
            }
            else
            {
                m_destroy_ticks = 0;
                m_is_destroying = false;
            }

            if (Input::is_action_just_pressed("interact"))
            {
                BlockState state = m_world->get_block_state(m_dimension, result.block_pos.x, result.block_pos.y, result.block_pos.z);
                std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);

                if (std::shared_ptr<InventoryBlock> ib = std::dynamic_pointer_cast<InventoryBlock>(block))
                {
                    // TODO: How to handle this with an RPC ?
                    ib->open_inventory(result.block_pos, this);
                }
                else
                {
                    ItemStack stack = m_inventory_container->get_stack(1, m_slot);
                    call_rpc("place_block", result.block_pos.x, result.block_pos.y, result.block_pos.z, result.normal, stack);
                }
            }
            if (Input::is_action_just_pressed("middle_click") && m_gamemode == GameMode::Creative)
            {
                BlockState state = m_world->get_block_state(m_dimension, result.block_pos.x, result.block_pos.y, result.block_pos.z);
                Id<Item> item = Engine::get().registry().to_item(state.id).value_or(Id<Item>());
                if (item.valid())
                {
                    ItemStack stack(item, 64);
                    m_inventory_container->set_stack(1, m_slot, stack);
                }
            }
        }
        else
        {
            m_aimed_block = std::nullopt;
            // Bow can be interacted even though he is not aiming at a block, can do better.
            if (Input::is_action_just_pressed("interact"))
            {
                ItemStack stack = m_inventory_container->get_stack(1, m_slot);
                if (stack.item().valid() && stack.item() == Items::bow)
                {
                    std::shared_ptr<Item> item = Engine::get().registry().get_item(stack.item());
                    item->interact(*m_world, m_dimension, stack, result.block_pos, result.normal, *m_inventory_container);
                    m_inventory_container->set_stack(1, m_slot, stack);
                }
            }
        }

        if (Input::is_action_just_released("interact"))
        {
            ItemStack stack = m_inventory_container->get_stack(1, m_slot);
            if (stack.item().valid())
            {
                std::shared_ptr<Item> item = Engine::get().registry().get_item(stack.item());
                item->on_release(*m_world, m_dimension, stack, m_camera->get_global_transform().position(), m_camera->get_global_transform().forward(), *m_inventory_container);
            }
        }
    }

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");
    const bool in_water = is_in_water();
    const bool chunk_loaded = chunk_is_loaded();

    if (m_local_player)
    {
        Renderer::get().set_underwater(head_in_water());
        if (head_in_water())
        {
            const glm::vec4 sky_color = glm::vec4(0.0, 0.0, 1.0, 1.0);
            Renderer::get().set_fog(sky_color, float(m_world->get_render_distance()) * 16.0f * 0.3f);
            Renderer::get().set_sky(sky_color);
        }
        else
        {
            const glm::vec4 sky_color = glm::vec4(130.0 / 255.0, 200.0 / 255.0, 229.0 / 255.0, 1.0);
            Renderer::get().set_fog(sky_color, float(m_world->get_render_distance()) * 16.0f - 1.0f);
            Renderer::get().set_sky(sky_color);
        }
    }

    float updown_dir = 0.0;
    if (are_input_available() && m_local_player && (!has_gravity() || in_water))
    {
        updown_dir = Input::get_axis("down", "jump");
    }

    float movement_damp = in_water ? 1.0f : 1.0f;
    float vertical_movement_damp = in_water ? 1.0f : 1.0f;

    if (are_input_available() && (glm::length2(dir) != 0.0 || updown_dir != 0.0) && m_local_player) //  && chunk_loaded)
    {
        float speed = m_speed;
        if (Input::is_action_pressed("sprint"))
            speed = m_sprint_speed;
        if (m_gamemode == GameMode::Creative)
            speed *= m_fly_speed_mult;

        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * glm::vec3(movement_damp, vertical_movement_damp, movement_damp) * speed;
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

    bool has_moved = m_velocity.x != 0 || m_velocity.z != 0;

    // Add some head bobbing
    if (m_gamemode == GameMode::Survival && (m_velocity.x != 0.0 || m_velocity.z != 0.0))
    {
        m_camera->get_transform().position().y = std::lerp(m_camera->get_transform().position().y, head_height + m_target_head_height, 0.5f);
        if (m_camera->get_transform().position().y == head_height + m_target_head_height)
            m_target_head_height = -m_target_head_height;
    }

    // Reset velocity after movements.
    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (has_gravity() && !in_water)
        m_velocity.y = std::clamp(m_velocity.y, -25.0, 25.0);
    else
        m_velocity.y = 0.0;

    if (!m_local_player && has_moved)
    {
        m_animator.play("walk");
        m_animator.tick(delta);
    }
    else if (!m_local_player)
    {
        m_animator.play("idle");
        m_animator.tick(delta);
    }

    if (m_local_player)
    {
        m_inventory->set_selected_slot(m_slot);

        if (m_opened_inventory.has_value())
            m_opened_inventory.value()->update_everything(delta);
        else
            m_inventory->update_everything(delta);
    }

    m_previous_frame_in_water = in_water;

    if (m_chat_opened)
    {
        m_chat->update_everything(delta);
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

void Player::draw(const RenderPass& pass)
{
    if (!m_local_player)
    {
        m_model->encode(pass, get_global_transform());
    }

    // if (m_local_player && m_aimed_block.has_value())
    // {
    //     SimpleUniforms uniforms(glm::translate(glm::identity<glm::mat4>(), glm::vec3(m_aimed_block.value())) * glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.01f)), glm::vec4(aim_color, 0.4));
    //     m_aim_buffer->update(View(uniforms).as_bytes());
    //     Renderer::get().draw(pass, Renderer::get().get_cube_mesh(), m_aim_material);
    // }

    if (m_local_player && m_inventory_container->get_stack(1, m_inventory->selected_slot()).item().valid())
    {
        Id<Item> id = m_inventory_container->get_stack(1, m_inventory->selected_slot()).item();
        std::shared_ptr<Item> item = Engine::get().registry().get_item(id);
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
        {
            std::shared_ptr<Block> block = Engine::get().registry().block_from_item(m_inventory_container->get_stack(1, m_inventory->selected_slot()).item());

            Transform3D transform;
            transform.scale() = glm::vec3(0.2);
            transform.position() = glm::vec3(0.32, -0.3, -0.4);

            glm::mat4 matrix = transform.to_matrix();

            std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_fw_item_block_shader());
            bg->set_param("camera", Renderer::get().get_fw_camera_rel());
            bg->set_param("model", m_model_buffer);
            bg->set_param("world_env", Renderer::get().get_fw_world_env());
            bg->set_param("images", EXPECT(Engine::get().registry().get_texture_array()->get_view(WGPUTextureViewDimension_2DArray)));
            bg->set_param("shadowmap", EXPECT(Renderer::get().get_fw_shadowmap()->get_view(WGPUTextureViewDimension_2D)));

            ItemBlockModel model(matrix,
                                 glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16)));
            m_model_buffer->update_struct(model);

            Renderer::get().draw(pass, Renderer::get().get_cube_mesh(), Renderer::get().get_fw_item_block_mat(), bg);
        }
        else
        {
            std::shared_ptr<Texture> texture = item->get_texture();

            Transform3D transform;
            transform.scale() = glm::vec3(0.2);
            transform.position() = glm::vec3(0.32, -0.18, -0.4);
            transform.set_euler_angles(glm::vec3(0, 90.0, 0));

            ItemBlockModel matrix(transform.to_matrix());
            m_model_buffer->update_struct(matrix);

            std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_fw_item_shader());
            bg->set_param("camera", Renderer::get().get_fw_camera_rel());
            bg->set_param("model", m_model_buffer);
            bg->set_param("image", EXPECT(texture->get_view(WGPUTextureViewDimension_2D)));

            Renderer::get().draw(pass, Renderer::get().get_quad_mesh(), Renderer::get().get_fw_item_mat(), bg);
        }
    }
}

void Player::draw_ui(const RenderPass& pass)
{
    if (m_local_player)
    {
        if (m_opened_inventory.has_value())
            m_opened_inventory.value()->draw_everything(pass);
        else
            m_inventory->draw_toolbar(pass);

        if (m_chat_opened)
        {
            m_chat->draw_everything(pass);
        }
    }
}

void Player::process_event(Event& event)
{
    if (m_chat_opened)
        m_chat->process_everyting(event);
}

Result<void> Player::save(EntitySerializer& ser) const
{
    int64_t gamemode = (int64_t)m_gamemode;
    ser.set("gamemode", gamemode);

    std::vector<ItemStack> stacks;
    stacks.resize(27 + 9);

    const InventoryContainer::Layer& layer = m_inventory_container->get_layer(0);
    for (size_t i = 0; i < 27; i++)
        stacks[i] = layer.stacks[i];

    const InventoryContainer::Layer& toolbar_layer = m_inventory_container->get_layer(1);
    for (size_t i = 0; i < 9; i++)
        stacks[i + 27] = toolbar_layer.stacks[i];

    Variant array = std::span(stacks);
    ser.set("inventory_data", array);

    return Result<void>();
}

Result<void> Player::load(const EntitySerializer& deser)
{
    int64_t gamemode = (int64_t)deser.get<int64_t>("gamemode").value_or(0);
    if (gamemode != 0 && gamemode != 1)
        gamemode = 0;
    set_gamemode((GameMode)gamemode);

    std::vector<ItemStack> stacks = deser.get_array<ItemStack>("inventory_data").value();

    if (stacks.size() != 27 + 9)
        return Error(ErrorKind::ReadFailure);

    InventoryContainer::Layer& layer = m_inventory_container->get_layer(0);
    for (size_t i = 0; i < 27; i++)
        layer.stacks[i] = stacks[i];

    InventoryContainer::Layer& toolbar_layer = m_inventory_container->get_layer(1);
    for (size_t i = 0; i < 9; i++)
        toolbar_layer.stacks[i] = stacks[i + 27];

    return Result<void>();
}

void Player::die()
{
    println("`{}` is dead", m_username);
}

void Player::break_block(int64_t x, int64_t y, int64_t z)
{
    BlockState state = m_world->get_block_state(m_dimension, x, y, z);
    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
    if (!block || block->is_unbreakable())
        return;

    if (m_gamemode == GameMode::Survival)
    {
        m_world->break_block(m_dimension, x, y, z);
    }
    else
    {
        m_world->set_block_state(m_dimension, x, y, z, BlockState());
    }
}

void Player::place_block(int64_t x, int64_t y, int64_t z, glm::vec3 normal, ItemStack stack)
{
    if (stack.item().valid())
    {
        std::shared_ptr<Item> item = Engine::get().registry().get_item(stack.item());
        item->interact(*m_world, m_dimension, stack, glm::i64vec3(x, y, z), normal, *m_inventory_container);
        if (m_local_player)
            m_inventory_container->set_stack(1, m_slot, stack);
    }
}

void Player::open_inventory(std::shared_ptr<Inventory> inventory)
{
    m_opened_inventory = inventory;
    Input::set_mouse_grabbed(false);
}

void Player::close_inventory()
{
    if (m_opened_inventory.has_value())
        m_opened_inventory.value()->grab_cancel();
    m_opened_inventory = std::nullopt;
    Input::set_mouse_grabbed(true);
}

bool Player::head_in_water() const
{
    return m_world->get_dimension(m_dimension).get_tag(get_position() + glm::dvec3(0, 1.2, 0.0), "water").has_value();
}
