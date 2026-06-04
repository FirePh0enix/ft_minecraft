#include "Engine.hpp"

#include "Args.hpp"
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
#include "Input.hpp"
#include "Inventory/Inventory.hpp"
#include "Network/Network.hpp"
#include "Network/Packet.hpp"
#include "Profiler.hpp"
#include "Render/Graph.hpp"
#include "Render/ImGUIToolKit.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <ctime>
#include <imgui.h>

Engine::Engine(const Args& args)
{
    singleton = this;
    m_window = EXPECT(newref<Window>("ft_minecraft", 1280, 720));

    Input::init(*m_window);
    Input::load_config();

    InitFlags flags;
    if (args.has("enable-gpu-validation"))
    {
        flags |= InitFlagBits::Validation;
    }

    register_entities(); // TODO: put this in GameRegistry

    EXPECT(m_renderer.init(*m_window, flags));

    m_depth_texture = EXPECT(Texture::create(1280, 720, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment));
    m_color_texture = EXPECT(Texture::create(1280, 720, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_RenderAttachment));

    recreate_graph();

    EXPECT(Font::init_library());
    m_font = EXPECT(Font::create("assets/fonts/Anonymous.ttf", 32));

    m_console.register_command("tp", EXPECT(Vector<CmdArgInfo>::create(CmdArgInfo(CmdArgKind::Int, "x"), CmdArgInfo(CmdArgKind::Int, "y"), CmdArgInfo(CmdArgKind::Int, "z"))),
                               [](const Command& cmd)
                               { Engine::get().get_player()->set_position(glm::vec3(cmd.get_arg_int("x"), cmd.get_arg_int("y"), cmd.get_arg_int("z"))); });

    m_console.register_command("gamemode", EXPECT(Vector<CmdArgInfo>::create(CmdArgInfo(CmdArgKind::String, "gamemode"))),
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

void Engine::register_entities()
{
    Entity::bind_methods();

    m_entity_registry.register_entity<Player>();
    m_entity_registry.register_entity<Cow>();
}

void Engine::tick(float delta)
{
    ZoneScoped;

    Option<SDL_Event> event_opt;

    {
        ZoneScopedN("handle events");

        while ((event_opt = m_window->poll_event()))
        {
            SDL_Event event = event_opt.get();

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
                Result<void> result = Renderer::get().configure_surface(w, h, VSync::On);
                ERR_EXPECT_B(result, "Failed to configure the surface");

                m_depth_texture = EXPECT(Texture::create(w, h, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment));
                m_color_texture = EXPECT(Texture::create(w, h, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_RenderAttachment));
                recreate_graph();

                if (m_scene == EngineScene::World)
                    m_world->get_active_camera()->update_projection((float)w / (float)h);
            }
            break;
            case SDL_EVENT_KEY_DOWN:
            {
                if (event.key.key == SDLK_F3)
                    m_debug_menu = !m_debug_menu;

                Event event2(event);
                if (m_scene == EngineScene::World)
                    m_player->process_event(event2);
            }
            break;
            case SDL_EVENT_TEXT_INPUT:
            {
                Event event2(event);
                if (m_scene == EngineScene::World)
                    m_player->process_event(event2);
            }
            break;
            default:
                break;
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
            ImGuiIO& imgui_io = ImGui::GetIO();

            if (imgui_io.WantCaptureMouse || imgui_io.WantCaptureKeyboard)
            {
                continue;
            }

            Input::process_event(event);
        }
    }

    if (m_last_second_timer_time >= 1.0)
    {
        m_current_memory_usage = core::get_memory_usage();
        m_last_second_timer_time -= 1.0;
        // println("{}", Chunk::instances);
    }
    m_last_second_timer_time += delta;

    m_connection.tick();

    switch (m_scene)
    {
    case EngineScene::MainMenu:
        break;
    case EngineScene::WaitingForWorld:
        break;
    case EngineScene::World:
    {
        m_world->tick(delta);

        // if (is_server() && is_online())
        // {
        //     for (Ref<Entity> entity : m_world->get_dimension(0).get_entities())
        //     {
        //         UpdateEntityPacket p(entity->id(), entity->get_transform().position(), entity->get_transform().rotation());
        //         m_connection.broadcast(m_connection.create_packet(p));
        //     }
        // }
    }
    break;
    }

    Input::post_events();
}

void Engine::draw()
{
    Result<void> res;

    switch (m_scene)
    {
    case EngineScene::MainMenu:
        res = draw_main_menu();
        break;
    case EngineScene::World:
        res = draw_world_scene();
        break;
    default:
        break;
    }

    if (res.has_error())
    {
        println(stderr, "Draw operation failed with error:");
        res.error().print();
    }
}

Result<void> Engine::draw_main_menu()
{
    const Extent2D window_size = m_window->size();

    m_renderer.draw(m_graph, [this, window_size](const RenderPassNode& node)
                    {
        if (node.is_final_pass())
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

                imguitk_center_next_widget("Connect");
                if (ImGui::Button("Connect"))
                {
                    connect_to_remote_world();
                }
            }
            ImGui::End();
        } });

    return Result<void>();
}

Result<void> Engine::draw_world_scene()
{
    Renderer::get().draw(m_graph, m_world);
    return Result<void>();
}

void Engine::encode_debug_menu()
{
    if (!m_debug_menu)
        return;

    if (ImGui::Begin("Debug"))
    {
        String s = format("{}", FormatBin(Engine::singleton->get_memory_usage()));
        ImGui::Text("RAM  = %s", s.data());

        String s2 = format("{}", FormatBin(Renderer::get().get_device_memory_usage()));
        ImGui::Text("VRAM = %s", s2.data());

        Transform3D transform = m_player->get_global_transform();
        ImGui::Text("Position: %.2f %.2f %.2f", transform.position().x, transform.position().y, transform.position().z);

        ImGui::InputText("Cmd", m_console.get_buffer(), m_console.get_buffer_size(), ImGuiInputTextFlags_CallbackCompletion, [](ImGuiInputTextCallbackData *data) -> int
                         {
                             Engine *self = (Engine *)data->UserData;
                             if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
                             {
                                self->m_console.exec();
                             }
                             return 0; }, this);
    }
    ImGui::End();
}

float Engine::time()
{
    return float(clock()) / float(CLOCKS_PER_SEC);
}

void Engine::create_world_and_start()
{
    m_connection.set_connect_handler(&Engine::connect_server, this);
    m_connection.set_disconnect_handler(&Engine::disconnect_server, this);
    m_connection.set_packet_handler(&Engine::receive_server, this);

    uint64_t seed = StringView(m_world_seed_buf).parse_int<uint64_t>();
    String name = "unamed";

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

    m_player = EXPECT(newref<Player>());
    m_world->add_entity(World::overworld, m_player);

    if (m_world->is_player_saved("player"))
    {
        String path = format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), m_world->get_name(), "player");

        EntitySerializer serializer;
        EXPECT(serializer.load(path));

        glm::vec3 position = serializer.get<glm::vec3>("position").get();
        glm::quat rotation = serializer.get<glm::quat>("rotation").get();

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
    // cow->get_transform().position() = glm::vec3(0.0f, 2.0f, 0.0f);
    // m_world->add_entity(World::overworld, cow);

    m_scene = EngineScene::World;
    m_authority = RpcTarget::Server;
}

void Engine::connect_to_remote_world()
{
    m_connection.set_connect_handler(&Engine::connect_client, this);
    m_connection.set_disconnect_handler(&Engine::disconnect_client, this);
    m_connection.set_packet_handler(&Engine::receive_client, this);

    m_scene = EngineScene::WaitingForWorld;
    m_authority = RpcTarget::Client;
    EXPECT(m_connection.connect_to(m_connect_ip, m_connect_port));

    // After this point we are waiting for the server to send us information about the world.
}

void Engine::exit()
{
    m_connection.close();
    Font::deinit_library();

    m_renderer.deinit();
    Entity::cleanup();
}

void Engine::recreate_graph()
{
    // #ifndef __platform_macos
    //     Ref<RenderPassNode> depth_pass = EXPECT(newref<RenderPassNode>());
    //     depth_pass->set_depth_output(m_depth_texture);
    // #endif

    Ref<RenderPassNode> color_pass = EXPECT(newref<RenderPassNode>());
    color_pass->set_depth_output(m_depth_texture);
    color_pass->set_color_output(m_color_texture);
    color_pass->set_output_to_surface(true);
    color_pass->set_next(nullptr);

    // #ifndef __platform_macos
    //     color_pass->set_load_depth(true);
    //     depth_pass->set_next(color_pass);
    // #endif

    // #ifndef __platform_macos
    //     m_graph.set_root(depth_pass);
    // #else
    m_graph.set_root(color_pass);
    // #endif
}

void Engine::receive_client(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    Engine *self = (Engine *)user;
    (void)conn;
    (void)client;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = EXPECT(buffer.read<PacketType>());

    switch (type)
    {
    case PacketType::Init:
    {
        InitPacket p;
        EXPECT(deserialize(buffer, p));

        self->m_world = EXPECT(World::create_proxy(p.seed));
        self->m_scene = EngineScene::World;

        self->m_player = EXPECT(newref<Player>());
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

        self->m_world->add_entity(0, entity);
    }
    break;
    case PacketType::RemoveEntity:
    {
        RemoveEntityPacket p;
        EXPECT(deserialize(buffer, p));

        debug("remove entity (id = {})", (uint32_t)p.id);
        // FIXME
        // self->m_world->remove_entity(0, p.id);
    }
    break;
    case PacketType::UpdateEntity:
    {
        UpdateEntityPacket p;
        EXPECT(deserialize(buffer, p));

        Ref<Entity> entity = self->m_world->get_entity(p.id);
        if (entity.is_null())
            break;

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
        EXPECT(variants.reserve(p.args.size()));
        for (const Variant& v : p.args)
            EXPECT(variants.append(v));

        Option<RpcTarget> rpc = entity->get_rpc(p.name);
        if (!rpc.has_value())
            break;

        if (rpc == RpcTarget::Server)
            break;

        entity->call(p.name, variants);
    };
    break;
    case PacketType::ChunkData:
    {
        ChunkDataPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->m_world->get_dimension(0).has_chunk(p.x, p.z))
        {
            // SAFETY: we already checked if the chunk exists, there is no multithreading to mess things up.
            Ref<Chunk> chunk = self->m_world->get_dimension(0).get_chunk(p.x, p.z).get();
            std::memcpy(chunk->get_blocks(), p.blocks.data(), sizeof(BlockState) * p.blocks.size());
            std::memcpy(chunk->get_biomes(), p.biomes.data(), sizeof(Biome) * p.biomes.size());
        }
        else
        {
            Ref<Chunk> chunk = EXPECT(newref<Chunk>(&self->m_world->get_dimension(0), p.x, p.z));
            std::memcpy(chunk->get_blocks(), p.blocks.data(), sizeof(BlockState) * p.blocks.size());
            std::memcpy(chunk->get_biomes(), p.biomes.data(), sizeof(Biome) * p.biomes.size());

            for (size_t i = 0; i < Chunk::slice_count; i++)
                EXPECT(chunk->build_simple_mesh(i));

            self->m_world->add_chunk(p.x, p.z, chunk);
        }
    }
    break;
    default:
        break;
    }
}

void Engine::connect_client(void *, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
}

void Engine::disconnect_client(void *user, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
    Engine *self = (Engine *)user;

    self->m_scene = EngineScene::MainMenu;
    self->m_world = nullptr;
}

void Engine::receive_server(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    (void)conn;
    Engine *self = (Engine *)user;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = EXPECT(buffer.read<PacketType>());

    switch (type)
    {
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
    case PacketType::RpcCall:
    {
        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

        Ref<Entity> entity = self->m_world->get_entity(p.id);
        if (entity.is_null())
            break;

        Vector<Variant> variants;
        EXPECT(variants.reserve(p.args.size()));
        for (const Variant& v : p.args)
            EXPECT(variants.append(v));

        Option<RpcTarget> rpc = entity->get_rpc(p.name);
        if (!rpc.has_value())
            break;

        if (rpc == RpcTarget::Server || rpc == RpcTarget::Both)
            entity->call(p.name, variants);

        if (rpc == RpcTarget::Both || rpc == RpcTarget::Client)
        {
            conn.broadcast(conn.create_packet(p), client.peer());
        }
    };
    break;
    default:
        break;
    }
}

void Engine::connect_server(void *user, NetworkConnection& conn, const Client& client)
{
    Engine *self = (Engine *)user;

    EntityId id = World::next_id();
    const glm::vec3 spawn_position = glm::vec3(0, 100.0, 0);

    InitPacket p(self->m_world->seed(), id, spawn_position);
    conn.send(client.peer(), conn.create_packet(p));

    for (Ref<Entity> entity : self->m_world->get_dimension(0).get_entities())
    {
        Transform3D transform = entity->get_transform();
        AddEntityPacket p(transform.position(), transform.rotation(), entity->id(), entity->get_class_hash_code());
        conn.send(client.peer(), conn.create_packet(p));
    }

    for (const auto& [pos, chunk] : self->m_world->get_dimension(0).get_chunks())
    {
        ChunkDataPacket chunk_packet;
        chunk_packet.x = pos.x;
        chunk_packet.z = pos.z;

        EXPECT(chunk_packet.blocks.resize(Chunk::block_count));
        std::memcpy(chunk_packet.blocks.data(), chunk->get_blocks(), sizeof(BlockState) * chunk_packet.blocks.size());

        EXPECT(chunk_packet.biomes.resize(Chunk::block_count));
        std::memcpy(chunk_packet.biomes.data(), chunk->get_biomes(), sizeof(Biome) * chunk_packet.biomes.size());

        conn.send(client.peer(), conn.create_packet(chunk_packet));
    }

    Engine::singleton->connection().broadcast(Engine::singleton->connection().create_packet(p));

    Ref<Player> player = EXPECT(newref<Player>());
    player->set_remote();
    player->set_id(id);
    player->get_transform().position() = spawn_position;

    self->m_world->add_entity(0, player);
    EXPECT(self->m_players.put(client.peer(), player));

    AddEntityPacket p2(player->get_transform().position(), player->get_transform().rotation(), id, player->get_class_hash_code());
    conn.broadcast(conn.create_packet(p2), client.peer());
}

void Engine::disconnect_server(void *user, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
    Engine *self = (Engine *)user;

    Ref<Player> player = self->m_players.get(client.peer()).get();

    RemoveEntityPacket p(player->id());
    conn.broadcast(conn.create_packet(p), client.peer());

    // FIXME
    // self->m_world->remove_entity(0, player->id());
}
