#include "Engine.hpp"

#include "Console.hpp"
#include "Core/Alloc.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Entity/Cow.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Player.hpp"
#include "Entity/Zombie.hpp"
#include "Input.hpp"
#include "Network/Network.hpp"
#include "Network/Packet.hpp"
#include "Profiler.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <cstddef>
#include <ctime>
#include <imgui.h>

constexpr int two_d_to_1d(int x, int y)
{
    return y * 3 + x;
}

#define WINDOW_INIT_WIDTH 1920
#define WINDOW_INIT_HEIGHT 1080

Engine::Engine()
{
    singleton = this;
    m_window = newref<Window>("ft_minecraft", WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT);

    Input::init(*m_window);
    Input::load_config();

    InitFlags flags = InitFlagBits::Validation;

    register_entities(); // TODO: put this in GameRegistry
    register_recipes();

    EXPECT(m_renderer.init(*m_window, flags));

    EXPECT(Font::init_library());
    m_font = EXPECT(Font::create("assets/fonts/Anonymous.ttf", 32));

    m_console.register_command("tp", Vector<CmdArgInfo>::create(CmdArgInfo(CmdArgKind::Int, "x"), CmdArgInfo(CmdArgKind::Int, "y"), CmdArgInfo(CmdArgKind::Int, "z")),
                               [](const Command& cmd)
                               { Engine::get().get_player()->set_position(glm::vec3(cmd.get_arg_int("x"), cmd.get_arg_int("y"), cmd.get_arg_int("z"))); });

    m_console.register_command("gamemode", Vector<CmdArgInfo>::create(CmdArgInfo(CmdArgKind::String, "gamemode")),
                               [](const Command& cmd)
                               {
                                   String mode = cmd.get_arg_string("gamemode");
                                   if (mode == "survival")
                                   {
                                       Engine::get().get_player()->set_gamemode(GameMode::Survival);
                                   }
                                   else if (mode == "creative")
                                   {
                                       Engine::get().get_player()->set_gamemode(GameMode::Creative);
                                   }
                               });
}

Engine::~Engine()
{
    m_connection.close();
    Font::deinit_library();
}

void Engine::register_entities()
{
    Entity::bind_methods();
    Player::bind_methods();

    m_entity_registry.register_entity<Player>();
    m_entity_registry.register_entity<Cow>();
    m_entity_registry.register_entity<Zombie>();
}

// TODO: Create a helper for creating recipe maybe ?
void Engine::register_recipes()
{
    Recipe crafting_table;

    crafting_table.width = 2;
    crafting_table.height = 2;

    for (size_t i = 0; i < 9; i++)
        crafting_table.pattern.push_back(Id<Item>());

    crafting_table.pattern[two_d_to_1d(0, 0)] = Items::stone_block;
    crafting_table.pattern[two_d_to_1d(1, 0)] = Items::stone_block;
    crafting_table.pattern[two_d_to_1d(0, 1)] = Items::stone_block;
    crafting_table.pattern[two_d_to_1d(1, 1)] = Items::stone_block;

    crafting_table.result = ItemStack(Items::crafting_table_block, 1);

    m_registry.add_recipe(crafting_table);
}

void Engine::tick(float delta)
{
    ZoneScoped;

    Option<SDL_Event> event_opt;

    {
        ZoneScopedN("handle events");

        while ((event_opt = m_window->poll_event()))
        {
            SDL_Event event = event_opt.value();

            switch (event.type)
            {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                m_window->close();
            }
            break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                const uint32_t w = event.window.data1;
                const uint32_t h = event.window.data2;
                Renderer::get().configure_surface(w, h, VSync::On);

                if (m_scene == GameScene::World)
                    m_world->get_active_camera()->update_projection((float)w / (float)h);
            }
            break;
            case SDL_EVENT_KEY_DOWN:
            {
                Event event2(event);
                if (m_scene == GameScene::World)
                    m_player->process_event(event2);
            }
            break;
            case SDL_EVENT_TEXT_INPUT:
            {
                Event event2(event);
                if (m_scene == GameScene::World)
                    m_player->process_event(event2);
            }
            break;
            default:
                break;
            }

            // TODO: remove ImGui
            if (m_scene != GameScene::World)
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                ImGuiIO& imgui_io = ImGui::GetIO();

                if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
                {
                    continue;
                }
            }

            Input::process_event(event);
        }
    }

    m_current_tps++;
    if (m_last_second_timer_time >= 1.0)
    {
        m_current_memory_usage = core::get_memory_usage();
        m_tps = m_current_tps;
        m_current_tps = 0;
        m_last_second_timer_time -= 1.0;
    }
    m_last_second_timer_time += delta;

    m_connection.tick();

    switch (m_scene)
    {
    case GameScene::MainMenu:
        break;
    case GameScene::WaitingForWorld:
        break;
    case GameScene::World:
    {
        m_world->tick(delta);
    }
    break;
    }

    Input::post_events();

    if (m_time_pass)
    {
        m_ticks_since_start_of_day += 1 * m_tick_scale;
        if (m_ticks_since_start_of_day > ticks_per_day)
            m_ticks_since_start_of_day = 0;
    }
}

void Engine::draw(float delta)
{
    ZoneScoped;

    switch (m_scene)
    {
    case GameScene::MainMenu:
        draw_main_menu();
        break;
    case GameScene::World:
        draw_world_scene();
        break;
    default:
        break;
    }

    m_current_fps++;
    if (m_last_second_frame_time >= 1.0)
    {
        m_fps = m_current_fps;
        m_current_fps = 0;
        m_last_second_frame_time -= 1.0;
    }
    m_last_second_frame_time += delta;
}

void Engine::draw_main_menu()
{
    const Extent2D window_size = m_window->size();

    m_renderer.draw_legacy([this, window_size]()
                           {
            const float size_x = (float)window_size.width * 0.4f;
            const float size_y = (float)window_size.height * 0.6f;

            ImGui::SetNextWindowPos(ImVec2((float)window_size.width / 2 - size_x / 2, (float)window_size.height / 2 - size_y / 2));
            ImGui::SetNextWindowSize(ImVec2(size_x, size_y));
            if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoDecoration))
            {
                imguitk_center_next_widget("Main Menu");
                ImGui::Text("Main Menu");
                ImGui::NewLine();

                const char *types[] = {"flat", "normal"};
                if (ImGui::Combo("World Type", &m_main_menu_world_type, types, 2))
                {
                }

                ImGui::InputText("Seed", m_world_seed_buf, sizeof(m_world_seed_buf));

                imguitk_center_next_widget("Play");
                if (ImGui::Button("Play"))
                {
                    create_world_and_start();
                }

                ImGui::SameLine();
                if (ImGui::Button("Host"))
                {
                    EXPECT(m_connection.host(NetworkConnection::default_port));
                    create_world_and_start();
                }
                
                if (ImGui::InputText("Ip", m_connect_ip, sizeof(m_connect_ip)))
                {
                }
                if (ImGui::DragInt("Port", &m_connect_port))
                {
                }
		if (ImGui::InputText("Username", m_username, sizeof(m_username))) {
		}

                imguitk_center_next_widget("Connect");
                if (ImGui::Button("Connect"))
                {
                    connect_to_remote_world();
                }
            }
            ImGui::End(); });
}

void Engine::draw_world_scene()
{
    m_renderer.draw_forward(m_world);
}

float Engine::time()
{
    // TODO: replace this by the real time.
    return float(clock()) / float(CLOCKS_PER_SEC);
}

void Engine::create_world_and_start()
{
    m_connection.set_connect_handler(&Engine::connect_server, this);
    m_connection.set_disconnect_handler(&Engine::disconnect_server, this);
    m_connection.set_packet_handler(&Engine::receive_server, this);

    uint64_t seed = StringView(m_world_seed_buf).parse_int<uint64_t>();
    String name = "unamed";

    m_ticks_since_start_of_day = ticks_per_day / 2;

    if (Filesystem::exists(format("{}saves/{}", Filesystem::get_data_directory(), name)))
    {
        info("loading existing world `{}`", name);
        m_world = EXPECT(World::load(name));
    }
    else
    {
        info("creating world `{}` with seed {}", name, seed);
        m_world = EXPECT(World::create(name, seed, m_main_menu_world_type));
    }

    String username = "john";

    m_player = newref<Player>();
    m_player.cast_to<Player>()->set_username(username);
    m_world->add_entity(World::overworld, m_player);

    if (m_world->is_player_saved(username))
    {
        String path = format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), m_world->get_name(), username);

        EntitySerializer serializer;
        EXPECT(serializer.load(path));

        glm::vec3 position = serializer.get<glm::vec3>("position").value_or(m_world->get_spawn_position());
        glm::quat rotation = serializer.get<glm::quat>("rotation").value_or({});

        m_player->set_position(position);
        m_player->set_rotation(rotation);
        m_player->load(serializer);
    }
    else
    {
        m_player->get_transform().position() = m_world->get_spawn_position();
    }

    m_world->force_load_chunk_for(m_player->get_position());

    // Ref<Entity> cow = EXPECT(newref<Cow>());
    // cow->get_transform().position() = m_player->get_position();
    // m_world->add_entity(World::overworld, cow);

    // Ref<Entity> zombie = newref<Zombie>();
    // zombie->get_transform().position() = m_player->get_position();
    // m_world->add_entity(World::overworld, zombie);

    m_scene = GameScene::World;
    m_authority = RpcTarget::Server;
}

void Engine::connect_to_remote_world()
{
    m_connection.set_connect_handler(&Engine::connect_client, this);
    m_connection.set_disconnect_handler(&Engine::disconnect_client, this);
    m_connection.set_packet_handler(&Engine::receive_client, this);

    m_scene = GameScene::WaitingForWorld;
    m_authority = RpcTarget::Client;
    EXPECT(m_connection.connect_to(m_connect_ip, m_connect_port));
}

void Engine::receive_client(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    Engine *self = (Engine *)user;
    (void)client;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = buffer.read<PacketType>();

    switch (type)
    {
    case PacketType::Refused: {
	RefusedPacket p;
	EXPECT(deserialize(buffer, p));

	info("Connection error: {}", p.message);

	conn.close();
	self->m_world = nullptr;
	self->m_scene = GameScene::MainMenu;
    };
    break;
    case PacketType::Init:
    {
        InitPacket p;
        EXPECT(deserialize(buffer, p));

        self->m_world = EXPECT(World::create_proxy(p.seed));
        self->m_scene = GameScene::World;

        self->m_player = newref<Player>();
        self->m_player->set_id(p.id);
        self->m_player->get_transform().position() = p.position;

        self->m_world->add_entity(0, self->m_player);

        debug("Init packet received, entity id is {}, spawn at [{}, {}, {}]", p.id, p.position.x, p.position.y, p.position.z);
    }
    break;
    case PacketType::AddEntity:
    {
        AddEntityPacket p;
        EXPECT(deserialize(buffer, p));

        debug("new entity (class_id = {}, id = {})", p.class_id.value, (uint32_t)p.id);

        Ref<Entity> entity = EXPECT(self->m_entity_registry.create_entity(p.class_id));
        entity->set_id(p.id);
        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;

        if (Ref<Player> player = entity)
            player->set_remote();

        self->m_world->add_entity(World::overworld, entity);
    }
    break;
    case PacketType::RemoveEntity:
    {
        RemoveEntityPacket p;
        EXPECT(deserialize(buffer, p));

        debug("remove entity (id = {})", (uint32_t)p.id);
        self->m_world->remove_entity(World::overworld, p.id);
    }
    break;
    case PacketType::UpdateEntity:
    {
        UpdateEntityPacket p;
        EXPECT(deserialize(buffer, p));

        Ref<Entity> entity = self->m_world->get_entity(p.id);
        if (entity.is_null())
            break;

	// debug("update entity (id = {})", (uint32_t)p.id);
	
        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;
    }
    break;
    case PacketType::RpcCall:
    {
        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

        debug("call `{}` with {} args on entity {}", p.name, p.args.size(), (uint32_t)p.id);

        Ref<Entity> entity = self->m_world->get_entity(p.id);
        if (entity.is_null())
            break;

        Vector<Variant> variants;
        variants.reserve(p.args.size());
        for (const Variant& v : p.args)
            variants.append(v);

        Option<RpcTarget> rpc = entity->get_rpc(p.name);
        if (!rpc.has_value())
            break;

	// We are on the client, so skip call to server RPCs.
        if (rpc == RpcTarget::Server)
            break;

        entity->call(p.name, variants);
    };
    break;
    case PacketType::ChunkData:
    {
        ChunkDataPacket p;
        EXPECT(deserialize(buffer, p));
	
	self->m_world->deferred_receive_chunk(p);
    }
    break;
    default:
        break;
    }
}

void Engine::connect_client(void *user, NetworkConnection& conn, const Client& client)
{
    (void)client;
    Engine *self = (Engine *)user;

    debug("hello world");

    BonjourPacket p;
    p.username = self->m_username;
    conn.send(conn.create_packet(p));
}

void Engine::disconnect_client(void *user, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
    Engine *self = (Engine *)user;

    self->m_scene = GameScene::MainMenu;
    self->m_world = nullptr;

    info("Disconnected from the player");
}

void Engine::receive_server(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    Engine *self = (Engine *)user;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = buffer.read<PacketType>();

    switch (type)
    {
    case PacketType::Bonjour: {
	BonjourPacket p;
	EXPECT(deserialize(buffer, p));
	
	if (self->has_player_with_name(p.username)) {
	    RefusedPacket p2;
	    p2.message = format("`{}` is already connected", p.username);
	    conn.send(client.peer(), conn.create_packet(p2));
	    break;
	}
	
	EntityId id = World::next_id();
	const glm::vec3 spawn_position = glm::vec3(0, 100.0, 0);

	InitPacket init_p(self->m_world->seed(), id, spawn_position);
	conn.send(client.peer(), conn.create_packet(init_p));

	for (Ref<Entity> entity : self->m_world->get_dimension(0).get_entities())
	    {
		Transform3D transform = entity->get_transform();
		AddEntityPacket p2(transform.position(), transform.rotation(), entity->id(), entity->get_class_hash_code());
		conn.send(client.peer(), conn.create_packet(p2));
	    }

	Ref<Player> player = newref<Player>();
	player->set_remote();
	player->set_id(id);
	player->get_transform().position() = spawn_position;
	player->set_username(p.username);

	self->m_world->add_entity(0, player);
	self->m_players.put(client.peer(), player);

	AddEntityPacket p2(player->get_transform().position(), player->get_transform().rotation(), id, player->get_class_hash_code());
	conn.broadcast(conn.create_packet(p2), client.peer());
    };
    break;
    case PacketType::SendPlayerTransform:
    {
        SendPlayerTransformPacket p;
        EXPECT(deserialize(buffer, p));

        Ref<Entity> entity = self->m_world->get_entity(p.id);
        if (entity.is_null())
            break;

        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;
    }
    break;
    case PacketType::RequestChunk:
    {
	RequestChunkPacket p;
	EXPECT(deserialize(buffer, p));

	// TODO: handle different dimension here.
	self->m_world->request_chunk(client.peer(), 0, p.x, p.z);
    };
    break;
    case PacketType::RpcCall:
    {
        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

        Ref<Entity> entity = self->m_world->get_entity(p.id);
	debug("received RPC call on {}", (uint32_t)p.id);
        if (entity.is_null())
            break;

        Option<RpcTarget> rpc = entity->get_rpc(p.name);
        if (!rpc.has_value())
            break;

        if (rpc == RpcTarget::Server || rpc == RpcTarget::Both)
            entity->call(p.name, p.args);

        if (rpc == RpcTarget::Both || rpc == RpcTarget::Client)
            conn.broadcast(conn.create_packet(p), client.peer());
    };
    break;
    default:
        break;
    }
}

void Engine::connect_server(void *user, NetworkConnection& conn, const Client& client)
{
    (void)user;
    (void)conn;
    (void)client;
}

void Engine::disconnect_server(void *user, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
    Engine *self = (Engine *)user;

    Ref<Player> player = self->m_players.get(client.peer()).value();

    RemoveEntityPacket p(player->id());
    conn.broadcast(conn.create_packet(p), client.peer());

    self->m_world->remove_entity(World::overworld, player);
    self->m_players.erase(client.peer());
}
