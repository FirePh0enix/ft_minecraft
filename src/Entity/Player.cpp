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
#include "Network/LocalServer.hpp"
#include "Network/Network.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "Render/Renderer.hpp"
#include "UI/Widget.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <cmath>
#include <cstdint>
#include <imgui.h>

#include <memory>
#include <print>

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
        std::println("usage `/tp <x> <y> <z>`");
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
        std::println("usage `/dim <dimension>`");
        return;
    }

    if (args[1] == "overworld" || args[1] == "0")
    {
        if (player->get_dimension() != World::overworld)
        {
            player->get_world()->change_dimension(player->id(), World::overworld);
            std::println("switched to `overworld`");
        }
        else
            std::println("already in the `overworld` dimension");
    }
    else if (args[1] == "underworld" || args[1] == "1")
    {
        if (player->get_dimension() != World::underworld)
        {
            player->get_world()->change_dimension(player->id(), World::underworld);
            std::println("switched to `underworld`");
        }
        else
            std::println("already in the `underworld` dimension");
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
        std::println("usage `/give <item> [count]`");
        return;
    }
}

void BetterConsole::gamemode(Player *player, const std::vector<std::string>& args)
{
    if (args.size() != 2)
    {
        std::println("usage `/gamemode <survival|creative|0|1>`");
        return;
    }

    if (args[1] == "survival" || args[1] == "0")
    {
        player->call_rpc("set_gamemode", static_cast<int64_t>(GameMode::Survival));
    }
    else if (args[1] == "creative" || args[1] == "1")
    {
        player->call_rpc("set_gamemode", static_cast<int64_t>(GameMode::Creative));
    }
}

void Player::bind_methods()
{
    type.add_method("set_gamemode", &Player::apply_gamemode);
    expose_rpc<Player>("set_gamemode", RpcTarget::Both);

    type.add_method("set_movement_state", &Player::set_movement_state);
    expose_rpc<Player>("set_movement_state", RpcTarget::Both);

    type.add_method("play_one_shot_sound", &Player::play_one_shot_sound);
    expose_rpc<Player>("play_one_shot_sound", RpcTarget::Both);

    type.add_method("hit", &Player::hit);
    expose_rpc<Player>("hit", RpcTarget::Both);

    type.add_method("interact", &Player::interact);
    expose_rpc<Player>("interact", RpcTarget::Both);

    type.add_method("cancel_use", &Player::cancel_use);
    expose_rpc<Player>("cancel_use", RpcTarget::Both);

    type.add_method("release", &Player::release);
    expose_rpc<Player>("release", RpcTarget::Both);

    type.add_method("on_death", &Player::on_death);
    expose_rpc<Player>("on_death", RpcTarget::Both);

    type.add_method("respawn", &Player::respawn);
    expose_rpc<Player>("respawn", RpcTarget::Server);

    type.add_method("on_respawn", &Player::on_respawn);
    expose_rpc<Player>("on_respawn", RpcTarget::Both);
}

void Player::apply_gamemode(int64_t gamemode)
{
    if (gamemode != static_cast<int64_t>(GameMode::Survival) &&
        gamemode != static_cast<int64_t>(GameMode::Creative))
        return;
    set_gamemode(static_cast<GameMode>(gamemode));
}

Player::Player()
    : LivingEntity(2)
{
    m_aabb = AABBd(-glm::dvec3(0.35, 0.9, 0.35), glm::dvec3(0.35, 0.9, 0.35));

    m_inventory_container = std::make_shared<InventoryContainer>();
    m_inventory_container->add_layer(27); // main inventory
    m_inventory_container->add_layer(9);  // toolbar
    m_inventory_container->add_layer(4);  // Crafting Ingredients
    m_inventory_container->add_layer(1);  // Crafting Result

    give_spawn_equipment();
}

void Player::give_spawn_equipment()
{
    size_t bows = 0;
    size_t arrows = 0;
    size_t crafting_tables = 0;
    for (size_t layer : {1, 0})
        for (const ItemStack& stack : m_inventory_container->get_layer(layer).stacks)
        {
            if (stack.item() == Items::bow)
                bows += stack.count();
            if (stack.item() == Items::arrow)
                arrows += stack.count();
            if (stack.item() == Items::crafting_table_block)
                crafting_tables += stack.count();
        }
    if (bows == 0)
        m_inventory_container->add_item(Items::bow);
    if (crafting_tables == 0)
        m_inventory_container->add_item(Items::crafting_table_block);
    while (arrows < 64 && m_inventory_container->add_item(Items::arrow))
        ++arrows;
}

void Player::on_ready()
{
    m_hand_model_buffer = EXPECT(Buffer::create(sizeof(FwModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
    m_hand_item_bg = BindGroup::create(Renderer::get().get_model_noshadow_shader());
    m_hand_item_bg->set_param("camera", Renderer::get().get_fw_camera());
    m_hand_item_bg->set_param("world_env", Renderer::get().get_fw_world_env());
    m_hand_item_bg->set_param("model", m_hand_model_buffer);
    m_hand_item_bg->set_param("atlas", EXPECT(Engine::get().registry().get_atlas()->get_view()));

    AudioMixer& audio = m_world->audio();
    auto path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/step/cloth1.ogg");
    m_walking_clip.emplace(*audio.get_audio_mixer(), path);

    path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/entity/player/attack/knockback1.ogg");
    m_attacking_clip.emplace(*audio.get_audio_mixer(), path);

    path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/random/bow.ogg");
    m_bow_release_clip.emplace(*audio.get_audio_mixer(), path);

    path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/liquid/swim1.ogg");
    m_swimming_clip.emplace(*audio.get_audio_mixer(), path);

    path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/step/gravel3.ogg");
    m_destroying_clip.emplace(*audio.get_audio_mixer(), path);

    path = std::filesystem::absolute("data/resourcepacks/pixel-perfection/assets/minecraft/sounds/random/classic_hurt.ogg");
    m_dying_clip.emplace(*audio.get_audio_mixer(), path);

    m_audio_source.emplace(audio);
    m_audio_source->set_clip(&m_walking_clip.value());

    m_camera = std::make_shared<Camera>();
    m_camera->get_transform().position() = glm::vec3(0, 0.85, 0);
    add_child(m_camera);

    m_model = ModelLegacy::load("data/models/player.json").value_or({});
    m_animator.set_model(m_model);

    if (m_local_player)
    {
        m_inventory = std::make_shared<PlayerInventory>(m_inventory_container, this);

        auto& clip = Engine::get().music_player().get_biome_music(m_current_biome);
        Engine::get().music_player().crossfade_to(&clip, 2.0f, 1.0f);

        // m_breaks_textures[0] = EXPECT(Texture::load("assets/textures/breaks/0.png"));
        // m_breaks_textures[1] = EXPECT(Texture::load("assets/textures/breaks/1.png"));
        // m_breaks_textures[2] = EXPECT(Texture::load("assets/textures/breaks/2.png"));
        // m_breaks_textures[3] = EXPECT(Texture::load("assets/textures/breaks/3.png"));

        m_health_bar = std::make_shared<Widget>();
        m_health_bar->set_expand_horizontal(true);
        m_health_bar->set_expand_vertical(true);
        m_health_bar->set_alignment(ContainerAlignment::Bottom);
        m_health_bar->set_layout(ContainerLayout::Stack);

        std::shared_ptr<ColorRectWidget> background = std::make_shared<ColorRectWidget>();
        background->set_size(Point(Size::px(500), Size::px(40)));
        background->set_color(Color::rgb(50, 50, 50));
        m_health_bar->add_child(background);

        std::shared_ptr<ColorRectWidget> colored_rect = std::make_shared<ColorRectWidget>();
        colored_rect->set_size(Point(Size::px(500), Size::px(40)));
        colored_rect->set_color(Colors::red);
        m_health_bar->add_child(colored_rect);
    }
}

void Player::tick(float delta)
{
    ZoneScoped;

    Entity::tick(delta);

    if (m_dead)
    {
        m_velocity = glm::dvec3(0.0);
        m_death_animation_time += delta;
        if (!m_local_player)
            m_animator.tick(delta);
        m_audio_source->set_position(get_global_transform().position());
        return;
    }

    if (Input::is_action_just_pressed("attack") && !Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_chat_opened)
    {
        Input::set_mouse_grabbed(true);
        m_paused = false;
    }
    else if (Input::is_action_just_pressed("escape") && Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && m_local_player && !m_chat_opened)
    {
        Input::set_mouse_grabbed(false);
        m_paused = true;
    }
    else if (Input::is_action_just_pressed("escape") && m_local_player && m_opened_inventory.has_value())
    {
        close_inventory();
    }
    else if (Input::is_action_just_pressed("escape") && m_local_player && m_chat_opened)
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

        AudioListener& listener = Engine::get().audio_mixer().get_audio_listener();
        const Transform3D& camera_transform = m_camera->get_global_transform();

        listener.set_position(camera_transform.position());
        listener.set_forward(camera_transform.forward());
        listener.set_up(camera_transform.up());

        if (Input::is_action_just_pressed("toggle_debug_menu"))
            m_debug_menu_opened = !m_debug_menu_opened;
    }

    if (Engine::get().is_server())
    {
        size_t amount_added = 0;

        AABB item_box = get_aabb().translate(get_position()).grow(glm::vec3(0.5));
        std::vector<std::shared_ptr<Entity>> entities = m_world->get_dimension(m_dimension).cast_box(item_box);
        for (const std::shared_ptr<Entity>& entity : entities)
        {
            if (std::shared_ptr<ItemEntity> item = std::dynamic_pointer_cast<ItemEntity>(entity))
            {
                // Remote players have inventory storage but no inventory UI.
                if (!m_inventory_container->add_item(item->item()))
                    continue;

                const RemoveEntityPacket packet(item->id());
                Engine::get().server()->route_packet(NetworkConnection::create_packet(packet));
                m_world->remove_entity(m_dimension, item);
                amount_added++;
            }
        }

        if (amount_added > 0)
            sync_inventory();
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
        }
        else
        {
            m_aimed_block = std::nullopt;
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
            Renderer::get().set_fog(sky_color, float(16) * 16.0f * 0.3f); // TODO: render distance
            Renderer::get().set_sky(sky_color);
        }
        else
        {
            const glm::vec4 sky_color = glm::vec4(130.0 / 255.0, 200.0 / 255.0, 229.0 / 255.0, 1.0);
            Renderer::get().set_fog(sky_color, float(16) * 16.0f - 1.0f);
            Renderer::get().set_sky(sky_color);
        }

        const glm::ivec3 pos = m_transform.position();
        const int64_t cx = chunk_index(pos.x);
        const int64_t cz = chunk_index(pos.z);

        const auto& chunk = m_world->get_chunk(cx, cz);

        if (chunk.has_value())
        {
            int64_t x = local_coords(pos.x);
            int64_t z = local_coords(pos.z);

            Biome biome = chunk->get()->get_biomes()[x + z * 16];
            if (biome != m_current_biome)
            {
                // std::println("{} {} | {} {}", x, z, (uint16_t)m_current_biome, (uint16_t)biome);
                m_current_biome = biome;
                auto& clip = Engine::get().music_player().get_biome_music(biome);
                Engine::get().music_player().crossfade_to(&clip, 2.0f, 1.0f);
            }
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

    // Add some head bobbing
    if (m_gamemode == GameMode::Survival && (m_velocity.x != 0.0 || m_velocity.z != 0.0))
    {
        m_camera->get_transform().position().y = std::lerp(m_camera->get_transform().position().y, head_height + m_target_head_height, 0.5f);
        if (m_camera->get_transform().position().y == head_height + m_target_head_height)
            m_target_head_height = -m_target_head_height;
    }

    const bool is_moving = glm::length2(glm::vec2(m_velocity.x, m_velocity.z)) > 1e-6f;

    MovementSound movement_sound = MovementSound::None;
    if (is_moving)
    {
        if (is_in_water())
            movement_sound = MovementSound::Swimming;
        else if (m_on_ground)
            movement_sound = MovementSound::Walking;
    }
    if (movement_sound != m_movement_sound)
    {
        set_movement_state(static_cast<int64_t>(movement_sound));
        if (m_local_player)
            call_rpc("set_movement_state", static_cast<int64_t>(movement_sound));
    }

    // Reset velocity after movements.
    m_velocity.x = 0.0;
    m_velocity.z = 0.0;

    if (has_gravity() && !in_water)
        m_velocity.y = std::clamp(m_velocity.y, -25.0, 25.0);
    else
        m_velocity.y = 0.0;

    if (m_movement_sound != MovementSound::None)
        m_animator.play(m_movement_sound == MovementSound::Swimming ? "swim" : "walk");
    else
        m_animator.play("idle");

    m_animator.tick(delta);

    if (m_local_player)
    {
        m_inventory->set_selected_slot(m_slot);

        if (m_opened_inventory.has_value())
            m_opened_inventory.value()->update_everything(delta);
        else
            m_inventory->update_everything(delta);

        std::dynamic_pointer_cast<ColorRectWidget>(m_health_bar->get_children()[1])->set_size(Point(Size::px(int32_t(500.0f * ((float)m_health / (float)m_max_health))), Size::px(40)));
        m_health_bar->invalidate();
        m_health_bar->update_everything(delta);
    }

    m_previous_frame_in_water = in_water;

    if (m_local_player && Engine::get().is_client())
    {
        SendPlayerTransformPacket p{};
        p.id = m_id;
        p.position = get_global_transform().position();
        p.rotation = get_global_transform().rotation();
        Engine::get().server()->route_packet(NetworkConnection::create_packet(p));
    }

    m_audio_source->set_position(get_global_transform().position());
}

void Player::set_movement_state(int64_t state)
{
    m_movement_sound = static_cast<MovementSound>(state);
    if (m_movement_sound == MovementSound::Swimming)
        m_audio_source->set_clip(&m_swimming_clip.value());
    else if (m_movement_sound == MovementSound::Walking)
        m_audio_source->set_clip(&m_walking_clip.value());
    else
    {
        m_audio_source->stop();
        return;
    }
    m_audio_source->play();
}

void Player::play_one_shot_sound(int64_t sound)
{
    switch (static_cast<EntitySound>(sound))
    {
    case EntitySound::Attack:
        m_audio_source->play_one_shot(&m_attacking_clip.value(), 0.5f);
        m_animator.play_once("attack");
        break;
    case EntitySound::Destroying:
        m_audio_source->play_one_shot(&m_destroying_clip.value(), 0.5f);
        m_animator.play_once("destroy_block");
        break;
    case EntitySound::BowRelease:
        m_audio_source->play_one_shot(&m_bow_release_clip.value(), 0.5f);
        break;
    case EntitySound::Death:
        m_audio_source->play_one_shot(&m_dying_clip.value(), 1.0f);
        m_animator.play_once("death", true);
        break;
    case EntitySound::Groan:
        break;
    }
}

void Player::draw(const RenderPass& pass, bool shadowmap)
{
    Transform3D render_transform = get_global_transform();
    // Player rotations are view rotations. Invert them for the +Z-facing model.
    render_transform.rotation() = glm::conjugate(render_transform.rotation()) *
                                  glm::angleAxis(glm::radians(180.0), glm::dvec3(0.0, 1.0, 0.0));

    if (m_dead)
    {
        constexpr float fall_duration = 0.8f;
        const float t = glm::smoothstep(0.0f, 1.0f,
                                        std::min(m_death_animation_time / fall_duration, 1.0f));
        render_transform.rotation() *= glm::angleAxis(
            glm::radians(90.0 * (double)t), glm::dvec3(0.0, 0.0, 1.0));
        render_transform.position().y -= 0.5 * (double)t;
    }

    if (!m_local_player)
    {
        if (m_model)
            m_model->encode(pass, render_transform, shadowmap);
    }
    else if (shadowmap)
    {
        if (m_model)
            m_model->encode(pass, render_transform, true);
    }

    if (shadowmap)
        return;

    // Remote players do not create a PlayerInventory; only the local player's
    // first-person hand is rendered below.
    if (!m_local_player)
        return;

    ItemStack stack = m_inventory_container->get_stack(1, m_slot);
    if (stack.item().valid())
    {
        Id<Item> id = stack.item();
        std::shared_ptr<Item> item = Engine::get().registry().get_item(id);
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
        {
            std::shared_ptr<Block> block = Engine::get().registry().block_from_item(id);

            Transform3D transform;
            transform.scale() = glm::vec3(0.2);
            transform.position() = glm::vec3(glm::dvec3(0.32, -0.2, -0.4));

            glm::mat4 matrix = transform.to_matrix();

            std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_model_noshadow_shader());
            bg->set_param("camera", Renderer::get().get_fw_camera_rel());
            bg->set_param("model", m_hand_model_buffer);
            bg->set_param("world_env", Renderer::get().get_fw_world_env());
            bg->set_param("atlas", EXPECT(Engine::get().registry().get_atlas()->get_view()));

            FwModel model(matrix);
            m_hand_model_buffer->update_struct(model);

            Renderer::get().draw(pass, block->get_mesh(), Renderer::get().get_model_noshadow_mat(), bg);
        }
        else
        {
            std::shared_ptr<Texture> texture = item->get_texture(stack);

            Transform3D transform;
            transform.scale() = glm::vec3(0.25);
            transform.position() = glm::vec3(0.32, -0.18, -0.4);
            transform.set_euler_angles(glm::radians(glm::vec3(0, -40.0, 15.0)));

            FwModel matrix(transform.to_matrix());
            m_hand_model_buffer->update_struct(matrix);

            std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_fw_item_shader());
            bg->set_param("camera", Renderer::get().get_fw_camera_rel());
            bg->set_param("model", m_hand_model_buffer);
            bg->set_param("image", EXPECT(texture->get_view()));

            Renderer::get().draw(pass, Renderer::get().get_quad_mesh(), Renderer::get().get_fw_item_mat(), bg);
        }
    }
}

void Player::draw_ui(const RenderPass& pass)
{
    if (m_local_player)
    {
        if (m_dead)
        {
            death_screen();
            return;
        }

        if (m_opened_inventory.has_value())
            m_opened_inventory.value()->draw_everything(pass);
        else
            m_inventory->draw_toolbar(pass);

        if (m_chat_opened)
            chat();

        if (Input::is_action_pressed("show_player_list"))
            player_list();

        if (m_debug_menu_opened)
            debug_menu();

        m_health_bar->draw_everything(pass);

        if (m_paused)
            pause_menu();
    }
}

void Player::process_event(Event& event)
{
    ZoneScoped;

    if (!m_local_player || m_dead)
        return;

    if (!are_input_available())
    {
        if (m_using_slot.has_value())
            call_rpc("cancel_use");
        return;
    }

    if (event.is_action_pressed("attack"))
    {
        // Pitch belongs to the client-side camera and is not included in the
        // synchronized player transform, so the server needs this direction.
        call_rpc("hit", glm::dvec3(m_camera->get_global_transform().forward()));
    }
    if (event.is_action_pressed("interact"))
    {
        sync_inventory();
        call_rpc("interact", int64_t(m_slot), glm::dvec3(m_camera->get_global_transform().forward()));
    }
    else if (event.is_action_released("interact"))
    {
        call_rpc("release", int64_t(m_slot), glm::dvec3(m_camera->get_global_transform().forward()));
    }
}

std::expected<void, Error> Player::save(EntitySerializer& ser) const
{
    ZoneScoped;

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

    return std::expected<void, Error>();
}

std::expected<void, Error> Player::load(const EntitySerializer& deser)
{
    ZoneScoped;

    int64_t gamemode = (int64_t)deser.get<int64_t>("gamemode").value_or(0);
    if (gamemode != 0 && gamemode != 1)
        gamemode = 0;
    set_gamemode((GameMode)gamemode);

    std::vector<ItemStack> stacks = deser.get_array<ItemStack>("inventory_data").value();

    if (stacks.size() != 27 + 9)
        return std::unexpected(Error(ErrorKind::ReadFailure));

    InventoryContainer::Layer& layer = m_inventory_container->get_layer(0);
    for (size_t i = 0; i < 27; i++)
        layer.stacks[i] = stacks[i];

    InventoryContainer::Layer& toolbar_layer = m_inventory_container->get_layer(1);
    for (size_t i = 0; i < 9; i++)
        toolbar_layer.stacks[i] = stacks[i + 27];

    return std::expected<void, Error>();
}

void Player::die()
{
    if (m_dead)
        return;

    m_dead = true;
    info("`{}` is dead", m_username);
    call_rpc("on_death");
}

void Player::on_death()
{
    cancel_use();
    m_dead = true;
    m_death_animation_time = 0.0f;
    play_one_shot_sound(static_cast<int64_t>(EntitySound::Death));

    if (m_local_player)
        Input::set_mouse_grabbed(false);
}

void Player::respawn()
{
    if (!m_dead)
        return;

    m_health = m_max_health;
    set_position(m_world->get_spawn_position());
    give_spawn_equipment();
    call_rpc("on_respawn");
    sync_inventory();
}

void Player::on_respawn()
{
    m_dead = false;
    m_death_animation_time = 0.0f;
    m_animator.stop();

    if (m_local_player)
        Input::set_mouse_grabbed(true);
}

void Player::hit(glm::dvec3 direction)
{
    const float range = 4.0f;
    const double direction_length_squared = glm::length2(direction);
    if (!std::isfinite(direction_length_squared) || direction_length_squared <= 0.0)
        return;

    // Keep the eye position and reach authoritative; only the aiming direction
    // comes from the client because camera pitch is not otherwise synchronized.
    direction /= std::sqrt(direction_length_squared);
    const Ray ray(m_camera->get_global_transform().position(), direction);

    RaycastResult result;
    // Ignore the attacker so its own collision box cannot hide the target.
    if (m_world->raycast(m_dimension, ray, range, result, this))
    {
        int64_t x = result.block_pos.x;
        int64_t y = result.block_pos.y;
        int64_t z = result.block_pos.z;

        if (result.hit_entity && result.entity->id() != m_id)
        {
            if (auto mob = std::dynamic_pointer_cast<LivingEntity>(result.entity))
            {
                if (Engine::get().is_server())
                    mob->damage(1, id()); // TODO: tool-dependent damage.

                call_rpc("play_one_shot_sound", static_cast<int64_t>(EntitySound::Attack));
            }
        }
        else if (m_gamemode == GameMode::Creative && !result.hit_entity)
        {
            m_world->set_block_state(m_dimension, x, y, z, BlockState());
        }
        else if (m_gamemode == GameMode::Survival && !result.hit_entity)
        {
            call_rpc("play_one_shot_sound", static_cast<int64_t>(EntitySound::Destroying));

            // if (!m_is_destroying)
            // {
            //     m_is_destroying = true;
            //     m_destroy_block_pos = result.block_pos;
            // }
            // else if (m_destroy_block_pos != result.block_pos)
            // {
            //     m_is_destroying = false;
            //     m_destroy_ticks = 0;
            // }

            // m_destroy_ticks += 1;
            // if (m_destroy_ticks >= max_destroy_ticks)
            // {
            //     m_world->break_block(m_dimension, x, y, z);
            //     m_is_destroying = false;
            //     m_destroy_ticks = 0;
            // }

            m_world->break_block(m_dimension, x, y, z);
        }
        else
        {
            m_destroy_ticks = 0;
            m_is_destroying = false;
        }
    }
}

void Player::cancel_use()
{
    if (!m_using_slot.has_value())
        return;
    ItemStack stack = m_inventory_container->get_stack(1, *m_using_slot);
    stack.remove_tag("draw_start");
    m_inventory_container->set_stack(1, *m_using_slot, stack);
    m_using_slot.reset();
    m_using_stack = ItemStack();
}

void Player::interact(int64_t slot, glm::dvec3 direction)
{
    cancel_use();
    if (m_dead || slot < 0 || slot >= 9 || !std::isfinite(glm::length2(direction)) || glm::length2(direction) <= 0.0)
        return;
    const Ray ray(m_camera->get_global_transform().position(), glm::normalize(direction));

    RaycastResult result{};
    const bool raycast_hit = m_world->raycast(m_dimension, ray, 4.0f, result, this);
    if (raycast_hit && !result.hit_entity)
    {
        BlockState state = m_world->get_block_state(m_dimension, result.block_pos.x, result.block_pos.y, result.block_pos.z);
        auto block = Engine::get().registry().get_block(state.id);
        if (auto ib = std::dynamic_pointer_cast<InventoryBlock>(block))
        {
            if (m_local_player)
                ib->open_inventory(result.block_pos, this);
            return;
        }
    }

    ItemStack stack = m_inventory_container->get_stack(1, slot);
    if (!stack.item().valid() || stack.count() == 0)
        return;
    auto item = Engine::get().registry().get_item(stack.item());
    if (!item)
        return;
    item->interact(*m_world, m_dimension, stack, raycast_hit && !result.hit_entity, result, *m_inventory_container);
    m_using_slot = size_t(slot);
    m_using_stack = stack;
    // Draw times belong to this process, not to saved or synchronized inventory.
    // stack.remove_tag("draw_start");
    m_inventory_container->set_stack(1, slot, stack);
}

void Player::release(int64_t slot, glm::dvec3 direction)
{
    if (!m_using_slot.has_value())
        return;
    if (m_dead || slot < 0 || slot >= 9 || size_t(slot) != *m_using_slot ||
        !std::isfinite(glm::length2(direction)) || glm::length2(direction) <= 0.0)
    {
        cancel_use();
        return;
    }
    ItemStack stack = m_inventory_container->get_stack(1, slot);
    if (stack.item().valid() && stack.item() == m_using_stack.item() && stack.count() > 0)
    {
        auto item = Engine::get().registry().get_item(stack.item());
        if (auto start = m_using_stack.get_tag<int64_t>("draw_start"))
            stack.set_tag("draw_start", *start);
        item->on_release(*m_world, m_dimension, stack, m_camera->get_global_transform().position(),
                         glm::vec3(glm::normalize(direction)), *m_inventory_container, this);
        m_inventory_container->set_stack(1, slot, stack);
    }
    cancel_use();
}

void Player::open_inventory(std::shared_ptr<Inventory> inventory)
{
    if (m_using_slot.has_value())
        call_rpc("cancel_use");
    m_opened_inventory = inventory;
    Input::set_mouse_grabbed(false);
}

void Player::close_inventory()
{
    if (m_opened_inventory.has_value())
    {
        m_opened_inventory.value()->grab_cancel();
        if (!m_opened_inventory.value()->on_close())
            return;
    }
    m_opened_inventory = std::nullopt;
    Input::set_mouse_grabbed(true);
}

bool Player::head_in_water() const
{
    return m_world->get_dimension(m_dimension).get_tag(get_position() + glm::dvec3(0, 1.2, 0.0), "water").has_value();
}

void Player::update_player_list(const std::vector<std::string>& names)
{
    m_player_list = names;
}

void Player::send_message(std::string message)
{
    m_messages.push_back(message);
}

void Player::sync_inventory()
{
    if (!Engine::get().is_client())
    {
        if (auto server = std::dynamic_pointer_cast<LocalServer>(Engine::get().server()))
            server->sync_player_inventory(*this);
        return;
    }
    SyncInventory p;
    for (size_t i = 0; i < 27; i++)
        p.items.push_back(m_inventory_container->get_stack(0, i));
    for (size_t i = 0; i < 9; i++)
        p.items.push_back(m_inventory_container->get_stack(1, i));
    Engine::get().server()->route_packet(NetworkConnection::create_packet(p));
}

void Player::death_screen()
{
    const Extent2D window_size = Engine::get().window()->size();
    constexpr float size_x = 360.0f;
    constexpr float size_y = 150.0f;

    ImGui::SetNextWindowPos(
        ImVec2((float)window_size.width * 0.5f, (float)window_size.height * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    ImGui::SetNextWindowBgAlpha(0.9f);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Death Screen", nullptr, flags))
    {
        const char *message = "You died!";
        ImGui::SetCursorPosY(35.0f);
        ImGui::SetCursorPosX((size_x - ImGui::CalcTextSize(message).x) * 0.5f);
        ImGui::TextUnformatted(message);

        constexpr float button_width = 160.0f;
        ImGui::SetCursorPosY(82.0f);
        ImGui::SetCursorPosX((size_x - button_width) * 0.5f);
        if (ImGui::Button("Respawn", ImVec2(button_width, 38.0f)))
            call_rpc("respawn");
    }
    ImGui::End();
}

void Player::player_list()
{
    const Extent2D window_size = Engine::get().window()->size();
    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.3f;

    ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, 0));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    if (ImGui::Begin("Player List"))
    {
        for (std::string_view name : m_player_list)
            ImGui::LabelText("", "%s", name.data());
    }
    ImGui::End();
}

void Player::chat()
{
    const Extent2D window_size = Engine::get().window()->size();
    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.4f;

    ImGui::SetNextWindowPos(ImVec2(0, (float)window_size.height / 1.7f));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    ImGui::SetNextWindowFocus();
    if (ImGui::Begin("Chat"))
    {
        if (ImGui::BeginChild("Messages", ImVec2(size_x, (float)window_size.height * 0.32f)))
        {
            for (std::string_view name : m_messages)
                ImGui::LabelText("", "%s", name.data());
        }
        ImGui::EndChild();

        ImGui::InputText("", m_chat_buffer, 128);
        ImGui::SameLine();
        if (ImGui::Button(">"))
        {
            std::string msg = m_chat_buffer;
            if (msg.starts_with("/"))
            {
                m_console.process_command(this, msg.substr(1));
            }
            else
            {
                Engine::get().server()->send_message(msg);
                send_message(m_username + ": " + msg);
            }
            m_chat_buffer[0] = 0;
        }
    }
    ImGui::End();
}

void Player::debug_menu()
{
    const Extent2D window_size = Engine::get().window()->size();
    const float size_x = (float)window_size.width * 0.4f;
    const float size_y = (float)window_size.height * 0.4f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    ImGui::SetNextWindowFocus();
    if (ImGui::Begin("Debug"))
    {
        ImGui::LabelText("", "FPS: %zu", std::max((size_t)Engine::get().get_fps(), (size_t)25));
        ImGui::LabelText("", "Chunks: %zu", m_world->get_dimension(0).get_chunks().size());
        ImGui::LabelText("", "Blocks: %zu", m_world->get_dimension(0).count_blocks());
        ImGui::LabelText("", "Triangles: %zu", m_world->get_dimension(0).count_triangles());
        ImGui::LabelText("", "Position: %lf %lf %lf", m_transform.position().x, m_transform.position().y, m_transform.position().z);
        ImGui::LabelText("", "Biome: %s", biome_names[(size_t)m_current_biome % (size_t)Biome::Max]);
    }
    ImGui::End();
}

void Player::pause_menu()
{
    const Extent2D window_size = Engine::get().window()->size();
    const float size_x = (float)window_size.width * 0.6f;
    const float size_y = (float)window_size.height * 0.7f;

    ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
    ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
    if (ImGui::Begin("Pause"))
    {
        if (ImGui::Checkbox("Fullscreen", &Engine::get().get_fullscreen()))
        {
            Engine::get().window()->set_fullscreen(Engine::get().get_fullscreen());
        }

        imguitk_center_next_widget("Quit");
        if (ImGui::Button("Quit"))
        {
            Engine::get().go_to_main_menu();
        }
    }
    ImGui::End();
}
