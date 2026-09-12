#pragma once

#include "Network/Network.hpp"
#include "Network/Server.hpp"

/// A `RemoteServer` is created by a client to represent the connection to a remote server.
class RemoteServer final : public Server
{
public:
    RemoteServer(std::string_view username, std::string_view ip, uint16_t port);
    ~RemoteServer();

    virtual void start() override;
    virtual void tick() override;

    virtual void send_message(const std::string& message) override;

    virtual void route_packet(ENetPacket *packet) override;

    void receive_chunk(const ChunkDataPacket& p, std::shared_ptr<Chunk> chunk, std::stop_token);
    void queue_receive_chunk(const ChunkDataPacket& p);

private:
    std::string m_username;
    std::string m_ip;
    uint16_t m_port;

    std::vector<std::string> m_connected_players;

    NetworkConnection m_connection;

    daking::MPSC_queue<std::shared_ptr<Chunk>> m_chunk_queue;

    std::set<ChunkPos> m_requested_chunks; // TODO: Add dimension

    void update_player_list();

    static void receive(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client);
    static void connect(void *, NetworkConnection& conn, const Client& client);
    static void disconnect(void *, NetworkConnection& conn, const Client& client);
};
