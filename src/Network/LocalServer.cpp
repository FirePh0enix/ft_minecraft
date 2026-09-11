#include "Network/LocalServer.hpp"

#include "Core/ZLib.hpp"
#include "Engine.hpp"
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

    m_world = EXPECT(World::create(m_world_name, m_world_seed, 0, Engine::get().audio_mixer()));

    m_schedulers[World::overworld] = std::make_shared<GenScheduler>(m_world->get_dimension(World::overworld), std::make_shared<OverworldGen>(m_world_settings));
    m_schedulers[World::underworld] = std::make_shared<GenScheduler>(m_world->get_dimension(World::underworld), std::make_shared<UnderworldGen>(m_world_settings));

    m_player = std::make_shared<Player>();
    m_player->set_username(m_username);
    m_world->add_entity(World::overworld, m_player);
    m_world->set_player(m_player);

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

    m_schedulers[m_player->get_dimension()]->tick();
    m_world->tick(1.0 / 60.0);

    if (m_online)
    {
        for (const ChunkLoadRequest& req : m_load_requests)
        {
            auto chunk_opt = m_world->get_dimension(req.dimension).get_chunk(req.x, req.z);
            if (chunk_opt.has_value())
            {
                auto chunk = chunk_opt.value();
                Engine::get().get_thread_pool().submit([this, req, chunk](std::stop_token)
                                                       { send_chunk(req.peer, chunk); });
            }
            else
            {
                // TODO: chunk is loaded but requested by a client, so we load the chunk and send it when its ready.
                //       This will require to split chunks in two: chunks loaded or visible chunks.
            }
        }
        m_load_requests.clear();

        for (std::shared_ptr<Entity> entity : m_world->get_dimension(World::overworld).get_entities()) // TODO: do the same for all dimensions
        {
            UpdateEntityPacket p{};
            p.id = entity->id();
            p.position = entity->get_transform().position();
            p.rotation = entity->get_transform().rotation();
            route_packet(NetworkConnection::create_packet(p));
        }
    }
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

    std::vector<uint8_t> blocks_data;
    EXPECT(ZLib::deflate(std::as_bytes(std::span((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count)), blocks_data));

    BufferWriter writer;
    Dimension::write_tags(writer, chunk);
    std::vector<uint8_t> tags_data;
    EXPECT(ZLib::deflate(std::as_bytes(writer.buffer()), tags_data));

    ChunkDataPacket p;
    p.x = chunk->x();
    p.z = chunk->z();

    p.blocks.resize(blocks_data.size());
    memcpy(p.blocks.data(), blocks_data.data(), blocks_data.size());

    p.tags.resize(tags_data.size());
    memcpy(p.tags.data(), tags_data.data(), tags_data.size());

    m_connection.send(peer, NetworkConnection::create_packet(p));
}

void LocalServer::route_packet(ENetPacket *packet)
{
    ZoneScoped;

    if (m_online)
        m_connection.broadcast(packet);
}

bool LocalServer::has_connected_player(std::string_view name)
{
    return name == m_username || std::find_if(m_connected_peers.begin(), m_connected_peers.end(), [name](const auto& pair) -> bool
                                              { return pair.second->get_username() == name; }) != m_connected_peers.end();
}

void LocalServer::update_player_list()
{
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
        player->get_transform().position() = self->m_world->get_spawn_position();
        player->set_username(p.username);

        // TODO: Send inventory content, maybe with a separate packet.
        if (self->m_world->is_player_saved(p.username))
        {
            self->m_world->load_player(p.username, player);
        }

        InitPacket init_p(self->m_world->seed(), id, player->get_transform().position());
        conn.send(client.peer(), NetworkConnection::create_packet(init_p));

        for (std::shared_ptr<Entity> entity : self->m_world->get_dimension(0).get_entities())
        {
            Transform3D transform = entity->get_transform();
            AddEntityPacket p2(transform.position(), transform.rotation(), entity->id(), entity->get_class_hash_code());
            conn.send(client.peer(), NetworkConnection::create_packet(p2));
        }

        PlayerConnected p2(std::string(self->m_player->get_username()));
        conn.send(client.peer(), NetworkConnection::create_packet(p2));
        for (const auto& [peer, player] : self->m_connected_peers)
        {
            PlayerConnected p(std::string(player->get_username()));
            conn.send(client.peer(), NetworkConnection::create_packet(p));
        }

        self->m_world->add_entity(0, player);
        self->m_connected_peers[client.peer()] = player;

        AddEntityPacket p3(player->get_transform().position(), player->get_transform().rotation(), id, player->get_class_hash_code());
        conn.broadcast(NetworkConnection::create_packet(p3), client.peer());

        self->update_player_list();
    };
    break;
    case PacketType::SendPlayerTransform:
    {
        SendPlayerTransformPacket p;
        EXPECT(deserialize(buffer, p));

        std::shared_ptr<Entity> entity = self->m_world->get_entity(p.id);
        if (entity == nullptr)
            break;

        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;
    }
    break;
    case PacketType::RequestChunk:
    {
        RequestChunkPacket p{};
        EXPECT(deserialize(buffer, p));

        self->m_load_requests.push_back(ChunkLoadRequest(client.peer(), p.dimension, p.x, p.z));
    };
    break;
    case PacketType::RpcCall:
    {
        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

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

        self->m_world->remove_entity(World::overworld, player);
        self->m_connected_peers.erase(client.peer());

        PlayerDisconnected p2(std::string(player->get_username()));
        conn.broadcast(NetworkConnection::create_packet(p2));

        self->update_player_list();
    }
}
