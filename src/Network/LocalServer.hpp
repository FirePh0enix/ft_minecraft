#pragma once

#include "Network/Network.hpp"
#include "Network/Server.hpp"
#include "World/Dimension.hpp"

struct ChunkLoadRequest
{
    ENetPeer *peer;
    int dimension;
    int64_t x;
    int64_t z;
};

class LocalServer final : public Server
{
public:
    LocalServer(std::string_view username, std::string_view world_name, uint64_t world_seed, bool online);
    ~LocalServer();

    virtual void start() override;
    virtual void tick() override;

    virtual void send_message(const std::string& message) override;

    virtual void route_packet(ENetPacket *packet) override;

    void host();

    void send_chunk(ENetPeer *peer, std::shared_ptr<Chunk> chunk);

private:
    std::string m_username;

    std::string m_world_name;
    uint64_t m_world_seed;
    WorldSettings m_world_settings;

    std::array<std::shared_ptr<GenScheduler>, World::max_dimensions> m_schedulers;

    bool m_online = false;
    std::map<ENetPeer *, std::shared_ptr<Player>> m_connected_peers;
    NetworkConnection m_connection;

    std::vector<ChunkLoadRequest> m_load_requests;

    bool has_connected_player(std::string_view name);
    void update_player_list();

    static void receive(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect(void *, NetworkConnection& conn, const Client& client);
    static void disconnect(void *, NetworkConnection& conn, const Client& client);
};
