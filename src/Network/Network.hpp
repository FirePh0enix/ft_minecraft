#pragma once

#include "Core/String.hpp"
#include "Network/Packet.hpp"

#include <enet/enet.h>

#include <cstdint>

enum class ConnectionState
{
    Idle,
    Connection,
    Connected,
    Host,
};

class Client
{
public:
    Client(String ip, uint16_t port, ENetPeer *peer)
        : m_ip(ip), m_port(port), m_peer(peer)
    {
    }

    Client()
    {
    }

    String ip() const { return m_ip; }
    uint16_t port() const { return m_port; }
    ENetPeer *peer() const { return m_peer; }

private:
    String m_ip;
    uint16_t m_port = 0;
    ENetPeer *m_peer = nullptr;
};

class NetworkConnection
{
public:
    using ConnectHandler = std::function<void(void *, NetworkConnection& conn, const Client& client)>;
    using DisconnectHandler = std::function<void(void *, NetworkConnection& conn, const Client& client)>;
    using PacketHandler = std::function<void(void *, NetworkConnection& conn, ENetPacket *packet, const Client& client)>;

    static constexpr uint16_t default_port = 25566;

    NetworkConnection();

    Result<void> connect_to(String ip, uint16_t port = default_port);
    Result<void> host(uint16_t port, String ip = "0.0.0.0");

    /**
     * Send a packet to a connected peer.
     */
    void send(ENetPeer *peer, ENetPacket *packet);

    void send(ENetPacket *packet);

    void broadcast(ENetPacket *packet, ENetPeer *ignored_peer = nullptr);

    void tick();

    /**
     * Close the host connection.
     */
    void close();

    template <typename T>
    ENetPacket *create_packet(const T& p)
    {
        DataBuffer buffer;
        EXPECT(buffer.write(T::type));
        EXPECT(serialize(buffer, p));
        return enet_packet_create(buffer.data().data(), buffer.data().size(), 0);
    }

    void set_packet_handler(PacketHandler handler, void *user)
    {
        m_packet_handler = handler;
        m_packet_handler_user = user;
    }

    void set_connect_handler(ConnectHandler handler, void *user)
    {
        m_connect_handler = handler;
        m_connect_handler_user = user;
    }

    void set_disconnect_handler(DisconnectHandler handler, void *user)
    {
        m_disconnect_handler = handler;
        m_disconnect_handler_user = user;
    }

    ConnectionState state() const { return m_state; }

private:
    ConnectionState m_state = ConnectionState::Idle;
    std::map<ENetPeer *, Client> m_clients;

    size_t m_maximum_connection = 32;
    ENetAddress m_address{};
    ENetHost *m_host = nullptr;
    /**
     * Corresponds to the server a client is connected to.
     */
    ENetPeer *m_peer = nullptr;
    bool m_is_server = false;
    PacketHandler m_packet_handler;
    void *m_packet_handler_user = nullptr;
    ConnectHandler m_connect_handler;
    void *m_connect_handler_user = nullptr;
    DisconnectHandler m_disconnect_handler;
    void *m_disconnect_handler_user = nullptr;

    void tick_client();
    void tick_server();
};
