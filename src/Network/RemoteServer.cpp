#include "Network/RemoteServer.hpp"

#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Entity/Player.hpp"
#include "Profiler.hpp"

RemoteServer::RemoteServer(std::string_view username, std::string_view ip, uint16_t port)
    : m_username(username), m_ip(ip), m_port(port)
{
}

RemoteServer::~RemoteServer()
{
    m_connection.close();
}

void RemoteServer::start()
{
    m_connection.set_connect_handler(&RemoteServer::connect, this);
    m_connection.set_disconnect_handler(&RemoteServer::disconnect, this);
    m_connection.set_packet_handler(&RemoteServer::receive, this);

    EXPECT(m_connection.connect_to(m_ip, m_port));
}

static void add_neighbour_chunk(ChunkPos pos, std::set<ChunkPos>& chunks)
{
    for (const auto& p : {
             ChunkPos(pos.x - 1, pos.z),
             ChunkPos(pos.x + 1, pos.z),
             ChunkPos(pos.x, pos.z - 1),
             ChunkPos(pos.x, pos.z + 1),
         })
        chunks.insert(p);
}

void RemoteServer::tick()
{
    ZoneScoped;

    m_connection.tick();

    if (m_world != nullptr)
    {
        const glm::vec3 player_pos = m_player->get_global_transform().position();
        int64_t player_cx = int64_t(player_pos.x / 16);
        int64_t player_cz = int64_t(player_pos.z / 16);
        for (int64_t cx = -16; cx <= 16; cx++)
        {
            for (int64_t cz = -16; cz <= 16; cz++)
            {
                int64_t x = player_cx + cx;
                int64_t z = player_cz + cz;

                if (m_requested_chunks.contains(ChunkPos(x, z)) || m_world->get_dimension(m_player->get_dimension()).has_chunk(x, z))
                    continue;

                RequestChunkPacket p{};
                p.x = x;
                p.z = z;
                p.dimension = m_player->get_dimension();
                m_connection.send(NetworkConnection::create_packet(p));

                m_requested_chunks.insert(ChunkPos(x, z));
            }
        }

        std::set<ChunkPos> chunk_modified;

        std::shared_ptr<Chunk> chunk;
        while (m_chunk_queue.try_dequeue(chunk))
        {
            m_world->get_dimension(chunk->dimension()).add_chunk(chunk);
            m_requested_chunks.erase(chunk->pos());
            chunk_modified.insert(chunk->pos());
            add_neighbour_chunk(chunk->pos(), chunk_modified);
        }

        for (ChunkPos pos : chunk_modified)
        {
            m_world->get_dimension(World::overworld).queue_rebuild(pos);
        }

        m_world->tick(1.0 / 60.0);
    }
}

void RemoteServer::send_message(const std::string& message)
{
    ZoneScoped;

    std::string msg;
    msg += m_player->get_username();
    msg += ": ";
    msg += message;

    ChatMessage p(msg);
    route_packet(NetworkConnection::create_packet(p));
}

void RemoteServer::route_packet(ENetPacket *packet)
{
    ZoneScoped;

    m_connection.send(packet);
}

void RemoteServer::receive_chunk(const ChunkDataPacket& p, std::shared_ptr<Chunk> chunk, std::stop_token token)
{
    ZoneScoped;

    std::vector<uint8_t> blocks_data;
    EXPECT(ZLib::inflate_with_cancellation(token, std::as_bytes(std::span(p.blocks)), blocks_data));
    if (blocks_data.size() != sizeof(BlockState) * Chunk::block_count)
    {
        debug("received bad or corrupted blocks data for {} {}", p.x, p.z);
        return;
    }
    memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

    std::vector<uint8_t> tags_data;
    EXPECT(ZLib::inflate_with_cancellation(token, std::as_bytes(std::span(p.tags)), tags_data));

    BufferReader reader(tags_data.data(), tags_data.size());
    Dimension::read_tags(reader, chunk);

    m_chunk_queue.enqueue(chunk);
}

void RemoteServer::queue_receive_chunk(const ChunkDataPacket& p)
{
    ZoneScoped;

    // TODO: add dimension
    std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(&m_world->get_dimension(World::overworld), p.x, p.z);
    Engine::get().get_thread_pool().submit([this, p, chunk](std::stop_token token)
                                           { receive_chunk(p, chunk, token); });
}

void RemoteServer::update_player_list()
{
    ZoneScoped;

    std::vector<std::string> list;
    list.push_back(std::string(m_player->get_username()));
    for (const auto& name : m_connected_players)
        list.push_back(name);
    m_player->update_player_list(list);
}

void RemoteServer::receive(void *user, NetworkConnection& conn, ENetPacket *packet, const Client& client)
{
    ZoneScoped;

    RemoteServer *self = (RemoteServer *)user;
    (void)client;

    const void *data = packet->data;
    const size_t data_size = packet->dataLength;

    DataBuffer buffer((char *)data, data_size);
    PacketType type = buffer.read<PacketType>();

    switch (type)
    {
    case PacketType::Refused:
    {
        ZoneScopedN("packet Refused");

        RefusedPacket p;
        EXPECT(deserialize(buffer, p));

        error("Connection error: {}", p.message);

        conn.close();
        self->m_world = nullptr;

        Engine::get().go_to_main_menu();
    };
    break;
    case PacketType::Init:
    {
        ZoneScopedN("packet Init");

        InitPacket p;
        EXPECT(deserialize(buffer, p));

        self->m_world = EXPECT(World::create_proxy(0, Engine::get().audio_mixer()));

        self->m_player = std::make_shared<Player>();
        self->m_player->set_id(p.id);
        self->m_player->get_transform().position() = p.position;
        self->m_player->set_username(self->m_username);

        self->m_world->set_player(self->m_player);
        self->m_world->add_entity(World::overworld, self->m_player);

        debug("Init packet received, entity id is {}, spawn at [{}, {}, {}]", p.id.value(), p.position.x, p.position.y, p.position.z);
    }
    break;
    case PacketType::AddEntity:
    {
        ZoneScopedN("packet AddEntity");

        AddEntityPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->m_world == nullptr)
            break;

        debug("new entity (class_id = {}, id = {})", p.class_id.value, (uint32_t)p.id);

        std::shared_ptr<Entity> entity = EXPECT(Engine::get().entity_registry().create_entity(p.class_id));
        entity->set_id(p.id);
        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;

        if (std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(entity))
            player->set_remote();

        self->m_world->add_entity(World::overworld, entity);
    }
    break;
    case PacketType::RemoveEntity:
    {
        ZoneScopedN("packet RemoveEntity");

        RemoveEntityPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->m_world == nullptr)
            break;

        debug("remove entity (id = {})", (uint32_t)p.id);
        self->m_world->remove_entity(World::overworld, p.id);
    }
    break;
    case PacketType::UpdateEntity:
    {
        ZoneScopedN("packet UpdateEntity");

        UpdateEntityPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->m_world == nullptr)
            break;

        if (p.id == self->m_world->get_player()->id())
            break;

        std::shared_ptr<Entity> entity = self->m_world->get_entity(p.id);
        if (entity == nullptr)
            break;

        entity->get_transform().position() = p.position;
        entity->get_transform().rotation() = p.rotation;
    }
    break;
    case PacketType::RpcCall:
    {
        ZoneScopedN("packet RpcCall");

        RpcCallPacket p;
        EXPECT(deserialize(buffer, p));

        debug("call `{}` on entity {}", p.name, (uint32_t)p.id);

        if (self->m_world == nullptr)
            break;

        std::shared_ptr<Entity> entity = self->m_world->get_entity(p.id);
        if (entity == nullptr)
            break;

        std::vector<Variant> variants;
        variants.reserve(p.args.size());
        for (const Variant& v : p.args)
            variants.push_back(v);

        std::optional<RpcTarget> rpc = entity->get_rpc(p.name);
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
        ZoneScopedN("packet ChunkData");

        ChunkDataPacket p;
        EXPECT(deserialize(buffer, p));

        if (self->m_world == nullptr)
            return;

        self->queue_receive_chunk(p);
    }
    break;
    case PacketType::PlayerConnected:
    {
        PlayerConnected p;
        EXPECT(deserialize(buffer, p));

        self->m_connected_players.push_back(p.name);
        self->update_player_list();
    }
    break;
    case PacketType::PlayerDisconnected:
    {
        PlayerDisconnected p;
        EXPECT(deserialize(buffer, p));

        self->m_connected_players.erase(std::find(self->m_connected_players.begin(), self->m_connected_players.end(), p.name));
        self->update_player_list();
    }
    break;
    case PacketType::ChatMessage:
    {
        ChatMessage p;
        EXPECT(deserialize(buffer, p));

        if (self->m_player == nullptr)
            break;

        self->m_player->send_message(p.message);
    }
    break;
    default:
        break;
    }
}

void RemoteServer::connect(void *user, NetworkConnection& conn, const Client& client)
{
    (void)client;
    RemoteServer *self = (RemoteServer *)user;

    BonjourPacket p;
    p.username = self->m_username;
    conn.send(NetworkConnection::create_packet(p));
}

void RemoteServer::disconnect(void *user, NetworkConnection& conn, const Client& client)
{
    (void)conn;
    (void)client;
    RemoteServer *self = (RemoteServer *)user;

    self->m_world = nullptr;
    Engine::get().go_to_main_menu();

    info("Disconnected from the server");
}
