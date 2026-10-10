#include "Network/LocalServer.hpp"

#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Entity/Item.hpp"
#include "Entity/Player.hpp"

LocalServer::LocalServer(std::string_view username, std::string_view world_name, uint64_t world_seed, bool online)
    : m_username(username), m_world_name(world_name), m_world_seed(world_seed), m_online(online)
{
}

LocalServer::~LocalServer()
{
    m_connection.close();
}

void LocalServer::start()
{
    ZoneScoped;

    m_connection.set_connect_handler(&LocalServer::connect, this);
    m_connection.set_disconnect_handler(&LocalServer::disconnect, this);
    m_connection.set_packet_handler(&LocalServer::receive, this);

    if (std::filesystem::exists(Filesystem::get_data_directory() + "saves/" + m_world_name))
    {
        info("loading existing world `{}`", m_world_name);
        m_world = EXPECT(World::load(m_world_name, Engine::get().audio_mixer()));
    }
    else
    {
        info("creating new world `{}` with seed `{}`", m_world_name, m_world_seed);
        m_world = EXPECT(World::create(m_world_name, m_world_seed, 0, Engine::get().audio_mixer()));
    }

    m_schedulers[World::overworld] = std::make_shared<GenScheduler>(m_world->get_dimension(World::overworld), std::make_shared<OverworldGen>(m_world_settings));
    m_schedulers[World::underworld] = std::make_shared<GenScheduler>(m_world->get_dimension(World::underworld), std::make_shared<UnderworldGen>(m_world_settings));

    // m_player = std::make_shared<Player>();
    // m_player->set_username(m_username);
    // m_world->add_entity(World::overworld, m_player);
    // m_world->set_player(m_player);

    if (m_online)
    {
        host();
    }

    update_player_list();
}

void LocalServer::tick()
{
    ZoneScoped;

    m_connection.tick();

    std::array<std::vector<BlockPos>, World::max_dimensions> origins;
    const auto add_origin = [&](const std::shared_ptr<Player>& player)
    {
        const glm::dvec3 position = player->get_position();
        origins[player->get_dimension()].push_back(BlockPos(
            int64_t(std::floor(position.x)), int64_t(position.y), int64_t(std::floor(position.z))));
    };
    if (m_player)
        add_origin(m_player);
    else
        origins[World::overworld].push_back(BlockPos());
    for (const auto& [peer, player] : m_connected_peers)
        add_origin(player);
    for (int dimension = 0; dimension < int(World::max_dimensions); ++dimension)
    {
        const auto portal_origins = m_world->portal_generation_origins(dimension);
        origins[dimension].insert(origins[dimension].end(), portal_origins.begin(), portal_origins.end());
        m_schedulers[dimension]->tick(origins[dimension]);
    }
    m_world->tick(1.0 / 60.0);

    if (m_online)
    {
        std::vector<ChunkLoadRequest> deferred_requests;

        for (const ChunkLoadRequest& req : m_load_requests)
        {
            auto chunk_opt = m_world->get_dimension(req.dimension).get_chunk(req.x, req.z);
            if (chunk_opt.has_value())
            {
                send_chunk(req.peer, chunk_opt.value());
            }
            else
            {
                deferred_requests.push_back(req);
            }
        }
        m_load_requests.clear();
        m_load_requests.insert(m_load_requests.end(), deferred_requests.begin(), deferred_requests.end());

        for (int dimension = 0; dimension < int(World::max_dimensions); ++dimension)
        {
            for (const auto& entity : m_world->get_dimension(dimension).get_entities())
            {
                if (entity->get_dimension() != dimension)
                    continue;
                UpdateEntityPacket p{};
                p.id = entity->id();
                p.dimension = dimension;
                p.position = entity->get_transform().position();
                p.rotation = entity->get_transform().rotation();
                route_packet(NetworkConnection::create_packet(p));
            }
        }
    }

    // If it fails, it fails no need to crash the game.
    if (m_player != nullptr)
        (void)m_world->save_player(m_player);
    for (const auto& [peer, player] : m_connected_peers)
        (void)m_world->save_player(player);
}

void LocalServer::send_message(const std::string& message)
{
    ZoneScoped;

    std::string msg;
    msg += m_player->get_username();
    msg += ": ";
    msg += message;

    ChatMessage p(msg);
    route_packet(NetworkConnection::create_packet(p));
}

void LocalServer::host()
{
    EXPECT(m_connection.host(NetworkConnection::default_port));
}

void LocalServer::send_chunk(ENetPeer *peer, std::shared_ptr<Chunk> chunk)
{
    ZoneScoped;

    // Snapshot on the world thread; compression workers must not read mutable chunks.
    const auto bytes = std::as_bytes(std::span(chunk->get_blocks(), Chunk::block_count));
    std::vector<std::byte> blocks(bytes.begin(), bytes.end());
    BufferWriter writer;
    Dimension::write_tags(writer, chunk);
    std::vector<uint8_t> tags(writer.buffer().begin(), writer.buffer().end());
    ChunkDataPacket packet;
    packet.x = chunk->x();
    packet.z = chunk->z();
    packet.dimension = chunk->dimension();
    packet.revision = ++m_chunk_revision;
    Engine::get().get_thread_pool().submit(
        [this, peer, packet, blocks = std::move(blocks), tags = std::move(tags)](std::stop_token token) mutable
        {
            if (token.stop_requested())
                return;
            EXPECT(ZLib::deflate(std::span<const std::byte>(blocks), packet.blocks));
            EXPECT(ZLib::deflate(std::as_bytes(std::span(tags)), packet.tags));
            m_connection.send(peer, NetworkConnection::create_packet(packet));
        });
}

void LocalServer::route_packet(ENetPacket *packet)
{
    ZoneScoped;

    if (m_online)
        m_connection.broadcast(packet);
    else
        enet_packet_destroy(packet);
}

void LocalServer::sync_portal_chunks(int dimension, int64_t x, int64_t z)
{
    // Send the generated arrival portal even to clients already caching this area.
    for (int64_t cx = chunk_index(x - 2); cx <= chunk_index(x + 2); ++cx)
        for (int64_t cz = chunk_index(z - 2); cz <= chunk_index(z + 2); ++cz)
            if (const auto chunk = m_world->get_dimension(dimension).get_chunk(cx, cz))
                for (const auto& [peer, player] : m_connected_peers)
                    send_chunk(peer, *chunk);
}

float LocalServer::get_generation_progression()
{
    return m_schedulers[World::overworld]->get_generation_progression();
}

bool LocalServer::has_generation_started()
{
    return m_schedulers[World::overworld]->has_generation_started();
}

void LocalServer::set_gamemode(GameMode gamemode)
{
    if (m_player == nullptr)
        return;

    m_player->call_rpc("set_gamemode", static_cast<int64_t>(gamemode));
    for (const auto& [peer, player] : m_connected_peers)
        player->call_rpc("set_gamemode", static_cast<int64_t>(gamemode));
}

void LocalServer::sync_player_inventory(const Player& player)
{
    for (const auto& [peer, connected] : m_connected_peers)
    {
        if (connected.get() != &player)
            continue;
        SyncInventory packet;
        for (size_t layer : {0, 1})
            for (const ItemStack& stack : player.get_inventory_container()->get_layer(layer).stacks)
                packet.items.push_back(stack);
        m_connection.send(peer, NetworkConnection::create_packet(packet));
        return;
    }
}

void LocalServer::spawn_player()
{
    m_player = std::make_shared<Player>();
    m_player->set_username(m_username);

    if (!m_world->load_player(m_player->get_username(), m_player))
    {
        m_player->set_position(m_world->get_spawn_position());
    }

    m_player->give_spawn_equipment();
    m_world->add_entity(m_player->get_dimension(), m_player);
    m_world->set_player(m_player);

    update_player_list();
}

bool LocalServer::has_connected_player(std::string_view name)
{
    return name == m_username || std::find_if(m_connected_peers.begin(), m_connected_peers.end(), [name](const auto& pair) -> bool
                                              { return pair.second->get_username() == name; }) != m_connected_peers.end();
}

void LocalServer::update_player_list()
{
    if (m_player == nullptr)
        return;

    std::vector<std::string> list;
    list.push_back(std::string(m_player->get_username()));
    for (const auto& [peer, player] : m_connected_peers)
        list.push_back(std::string(player->get_username()));
    m_player->update_player_list(list);
}

void LocalServer::receive(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    LocalServer *self = (LocalServer *)user;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = buffer.read<PacketType>();

    switch (type)
    {
    case PacketType::Bonjour:
    {
        BonjourPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->has_connected_player(p.username))
        {
            RefusedPacket p2;
            p2.message = std::format("`{}` is already connected", p.username);
            conn.send(client.peer(), conn.create_packet(p2));
            conn.disconnect(client.peer());
            break;
        }

        EntityId id = World::next_id();

        std::shared_ptr<Player> player = std::make_shared<Player>();
        player->set_remote();
        player->set_id(id);
        player->set_username(p.username);

        if (!self->m_world->load_player(p.username, player))
            player->get_transform().position() = self->m_world->get_spawn_position();

        player->give_spawn_equipment();
        // The host's mode governs the session, overriding a joiner's saved mode.
        player->set_gamemode(self->m_player->get_gamemode());

        InitPacket init_p(self->m_world->seed(), id, player->get_transform().position());
        init_p.dimension = player->get_dimension();
        conn.send(client.peer(), NetworkConnection::create_packet(init_p));

        const auto gamemode_packet = [](const Player& player)
        {
            return RpcCallPacket{
                .id = player.id(),
                .name = "set_gamemode",
                .args = {static_cast<int64_t>(player.get_gamemode())},
            };
        };

        const auto send_entity = [&](const std::shared_ptr<Entity>& entity)
        {
            Transform3D transform = entity->get_transform();
            if (const auto item = std::dynamic_pointer_cast<ItemEntity>(entity))
            {
                const AddItemEntityPacket item_packet{
                    .position = transform.position(),
                    .id = entity->id(),
                    .item_id = item->item().hash,
                    .dimension = entity->get_dimension(),
                };
                conn.send(client.peer(), NetworkConnection::create_packet(item_packet));
                return;
            }

            AddEntityPacket p2(transform.position(), transform.rotation(), entity->id(), entity->get_class_hash_code(), entity->get_dimension());
            conn.send(client.peer(), NetworkConnection::create_packet(p2));

            if (const auto existing_player = std::dynamic_pointer_cast<Player>(entity))
            {
                conn.send(client.peer(), NetworkConnection::create_packet(gamemode_packet(*existing_player)));
                const RpcCallPacket health_packet{
                    .id = existing_player->id(),
                    .name = "sync_health",
                    .args = {existing_player->get_health()},
                };
                conn.send(client.peer(), NetworkConnection::create_packet(health_packet));
            }

            const int64_t movement_state = entity->get_movement_state();
            if (movement_state != 0)
            {
                RpcCallPacket movement_packet;
                movement_packet.id = entity->id();
                movement_packet.name = "set_movement_state";
                movement_packet.args.emplace_back(movement_state);
                conn.send(client.peer(), NetworkConnection::create_packet(movement_packet));
            }
        };

        // Send all existing entities, including pending spawns, to the newly connected player.
        for (int dim = 0; dim < int(World::max_dimensions); ++dim)
        {
            const Dimension& dimension = self->m_world->get_dimension(dim);
            for (const auto& entity : dimension.get_entities())
                if (entity->get_dimension() == dim)
                    send_entity(entity);
            for (const auto& entity : dimension.get_pending_entities())
                if (entity->get_dimension() == dim)
                    send_entity(entity);
        }

        PlayerConnected p2(std::string(self->m_player->get_username()));
        conn.send(client.peer(), NetworkConnection::create_packet(p2));
        for (const auto& [peer, player] : self->m_connected_peers)
        {
            PlayerConnected p(std::string(player->get_username()));
            conn.send(client.peer(), NetworkConnection::create_packet(p));
        }

        self->m_world->add_entity(player->get_dimension(), player);
        self->m_connected_peers[client.peer()] = player;
        self->sync_player_inventory(*player);

        AddEntityPacket p3(player->get_transform().position(), player->get_transform().rotation(), id, player->get_class_hash_code(), player->get_dimension());
        conn.broadcast(NetworkConnection::create_packet(p3), client.peer());

        // The joining client starts with a default Player. Apply the session's
        // mode on every peer before it can predict breaking blocks.
        conn.broadcast(NetworkConnection::create_packet(gamemode_packet(*player)));

        self->update_player_list();
    };
    break;
    case PacketType::SendPlayerTransform:
    {
        SendPlayerTransformPacket p;
        EXPECT(deserialize(buffer, p));

        const auto player_it = self->m_connected_peers.find(client.peer());
        if (player_it == self->m_connected_peers.end() ||
            player_it->second->id() != p.id || player_it->second->is_dead())
            break;

        std::shared_ptr<Entity> entity = self->m_world->get_entity(p.id);
        if (entity == nullptr || p.dimension != entity->get_dimension())
            break;

        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;
    }
    break;
    case PacketType::RequestChunk:
    {
        RequestChunkPacket p{};
        EXPECT(deserialize(buffer, p));

        if (p.dimension < 0 || p.dimension >= int(World::max_dimensions))
            break;
        self->m_load_requests.push_back(ChunkLoadRequest(client.peer(), p.dimension, p.x, p.z));
    };
    break;
    case PacketType::RpcCall:
    {
        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

        // Gamemode, health and dimension transfers are controlled by the server.
        if (p.name == "set_gamemode" || p.name == "sync_health" || p.name == "sync_dimension")
            break;

        std::shared_ptr<Entity> entity = self->m_world->get_entity(p.id);
        debug("received RPC call on {}", (uint32_t)p.id);
        if (entity == nullptr)
            break;

        std::optional<RpcTarget> rpc = entity->get_rpc(p.name);
        if (!rpc.has_value())
            break;

        if (rpc == RpcTarget::Server || rpc == RpcTarget::Both)
            entity->call(p.name, p.args);

        if (rpc == RpcTarget::Both || rpc == RpcTarget::Client)
            conn.broadcast(conn.create_packet(p), client.peer());
    };
    break;
    case PacketType::ChatMessage:
    {
        ChatMessage p;
        EXPECT(deserialize(buffer, p));

        if (self->m_player != nullptr)
            self->m_player->send_message(p.message);

        conn.broadcast(NetworkConnection::create_packet(p), client.peer());
    }
    break;
    case PacketType::SyncInventory:
    {
        SyncInventory p;
        EXPECT(deserialize(buffer, p));
        if (p.items.size() != 36)
            break;

        if (!self->m_connected_peers.contains(client.peer()))
            break;

        std::shared_ptr<Player> player = self->m_connected_peers[client.peer()];

        for (size_t i = 0; i < 27; i++)
            player->get_inventory_container()->set_stack(0, i, p.items[i]);
        for (size_t i = 0; i < 9; i++)
            player->get_inventory_container()->set_stack(1, i, p.items[i + 27]);
    }
    break;
    default:
        break;
    }
}

void LocalServer::connect(void *user, NetworkConnection& conn, const Client& client)
{
    (void)user;
    (void)conn;
    (void)client;
}

void LocalServer::disconnect(void *user, NetworkConnection& conn, const Client& client)
{
    ZoneScoped;

    LocalServer *self = (LocalServer *)user;

    if (self->m_connected_peers.contains(client.peer()))
    {
        std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(self->m_connected_peers.at(client.peer()));

        RemoveEntityPacket p(player->id());
        conn.broadcast(NetworkConnection::create_packet(p), client.peer());

        self->m_world->remove_entity(player->get_dimension(), player);
        self->m_connected_peers.erase(client.peer());

        PlayerDisconnected p2(std::string(player->get_username()));
        conn.broadcast(NetworkConnection::create_packet(p2));

        self->update_player_list();
    }
}
