#include "Network.hpp"

#include "Core/Assert.hpp"
#include "Core/Error.hpp"
#include "Core/Logger.hpp"

#include <enet/enet.h>

NetworkConnection::NetworkConnection()
{
    ASSERT(enet_initialize() == 0, "failed to initialize ENet");
}

Result<void> NetworkConnection::connect_to(String ip, uint16_t port)
{
    m_address.port = port;
    enet_address_set_host_ip(&m_address, "127.0.0.1");

    m_host = enet_host_create(nullptr, 1, 0, 0, 0);
    if (m_host == nullptr)
    {
        error("Failed to create client");
        return Error(ErrorKind::ConnectionFailed);
    }

    m_peer = enet_host_connect(m_host, &m_address, 1, 0);
    if (m_peer == nullptr)
    {
        error("Failed to connect to {}:{}", ip, port);
        return Error(ErrorKind::ConnectionFailed);
    }

    debug("Trying to connect to {}:{}", ip, port);

    m_state = ConnectionState::Connection;
    m_is_server = false;
    return Result<void>();
}

Result<void> NetworkConnection::host(uint16_t port, String ip)
{
    (void)ip;
    m_address.port = port;
    m_address.host = ENET_HOST_ANY;

    m_host = enet_host_create(&m_address, m_maximum_connection, 0, 0, 0);
    if (m_host == nullptr)
    {
        error("Failed to host on *:{}", port);
        return Error(ErrorKind::HostCreationFailed);
    }

    info("Hosting on *:{}", port);

    m_state = ConnectionState::Host;
    m_is_server = true;
    return Result<void>();
}

void NetworkConnection::send(ENetPeer *peer, ENetPacket *packet)
{
    enet_peer_send(peer, 0, packet);
}

void NetworkConnection::send(ENetPacket *packet)
{
    send(m_peer, packet);
}

void NetworkConnection::broadcast(ENetPacket *packet, ENetPeer *peer)
{
    for (const auto& [_, key, value] : m_clients)
    {
        if (peer == value.peer())
            continue;
        enet_peer_send(value.peer(), 0, packet);
    }
}

void NetworkConnection::tick()
{
    if (m_state == ConnectionState::Idle)
        return;

    if (m_is_server)
    {
        tick_server();
    }
    else
    {
        tick_client();
    }
}

void NetworkConnection::tick_client()
{
    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            m_state = ConnectionState::Connected;
            Client client("", 0, event.peer);
            m_connect_handler(m_connect_handler_user, *this, client);
            info("Connected to server");
        }
        break;
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            Client client("", 0, event.peer);
            m_disconnect_handler(m_disconnect_handler_user, *this, client);
            close();
        };
        break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            Client client("", 0, event.peer);
            m_packet_handler(m_packet_handler_user, *this, event.packet, client);
            enet_packet_destroy(event.packet);
        }
        break;
        default:
            break;
        }
    }
}

void NetworkConnection::tick_server()
{
    ENetEvent event;
    while (enet_host_service(m_host, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            char address_buf[32];
            enet_address_get_host_ip(&event.peer->address, address_buf, sizeof(address_buf));

            Client client(String(address_buf), event.peer->address.port, event.peer);
            m_clients.put(event.peer, client);

            m_connect_handler(m_disconnect_handler_user, *this, client);

            info("Client connected from {}:{}", client.ip(), client.port());
        }
        break;
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            m_disconnect_handler(m_disconnect_handler_user, *this, m_clients.get(event.peer).value());

            const Client& client = *m_clients.get_ptr(event.peer).value();
            info("Client disconnected from {}:{}", client.ip(), client.port());

            m_clients.erase(event.peer);
        };
        break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            const Client& client = *m_clients.get_ptr(event.peer).value();
            m_packet_handler(m_packet_handler_user, *this, event.packet, client);
            enet_packet_destroy(event.packet);
        }
        break;
        default:
            break;
        }
    }
}

void NetworkConnection::close()
{
    switch (m_state)
    {
    case ConnectionState::Host:
        enet_host_destroy(m_host);
        break;
    case ConnectionState::Connected:
    case ConnectionState::Connection:
    {
        if (m_state == ConnectionState::Connected)
            enet_peer_disconnect(m_peer, 0);
        enet_host_destroy(m_host);
    }
    break;
    default:
        break;
    }

    // Reset the state connection state to idle.
    m_state = ConnectionState::Idle;
}
