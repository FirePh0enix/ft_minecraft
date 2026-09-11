#include "Network.hpp"

#include "Core/Assert.hpp"
#include "Core/Error.hpp"
#include "Core/Logger.hpp"
#include "Profiler.hpp"

#include <enet/enet.h>

NetworkConnection::NetworkConnection()
{
    ASSERT(enet_initialize() == 0, "failed to initialize ENet");
}

NetworkConnection::~NetworkConnection()
{
    close();
}

std::expected<void, Error> NetworkConnection::connect_to(std::string_view ip, uint16_t port)
{
    ZoneScoped;

    m_address.port = port;
    enet_address_set_host_ip(&m_address, ip.data());

    m_host = enet_host_create(nullptr, 1, 0, 0, 0);
    if (m_host == nullptr)
    {
        error("Failed to create client");
        return std::unexpected(Error(ErrorKind::ConnectionFailed));
    }

    m_peer = enet_host_connect(m_host, &m_address, 1, 0);
    if (m_peer == nullptr)
    {
        error("Failed to connect to {}:{}", ip, port);
        return std::unexpected(Error(ErrorKind::ConnectionFailed));
    }

    debug("Trying to connect to {}:{}", ip, port);

    m_worker_state = true;
    m_worker_thread = std::thread(std::bind(&NetworkConnection::worker, this));

    m_state = ConnectionState::Connection;
    m_is_server = false;
    return std::expected<void, Error>();
}

std::expected<void, Error> NetworkConnection::host(uint16_t port, std::string_view ip)
{
    ZoneScoped;

    (void)ip;
    m_address.port = port;
    m_address.host = ENET_HOST_ANY;

    m_host = enet_host_create(&m_address, m_maximum_connection, 0, 0, 0);
    if (m_host == nullptr)
    {
        error("Failed to host on *:{}", port);
        return std::unexpected(Error(ErrorKind::HostCreationFailed));
    }

    info("Hosting on *:{}", port);

    m_worker_state = true;
    m_worker_thread = std::thread(std::bind(&NetworkConnection::worker, this));

    m_state = ConnectionState::Host;
    m_is_server = true;
    return std::expected<void, Error>();
}

void NetworkConnection::send(ENetPeer *peer, ENetPacket *packet)
{
    // ZoneScoped;

    // std::lock_guard<std::mutex> lock(m_host_mutex); // TODO: one mutex per client maybe
    // enet_peer_send(peer, 0, packet);

    ENetAction action{};
    action.type = ENetAction::TYPE_PACKET;
    action.packet.packet = packet;
    action.packet.broadcast = false;
    action.packet.filter = peer;
    m_action_queue.enqueue(action);

    // PacketInfo p(packet, false, peer);
    // m_packet_queue.enqueue(p);
}

void NetworkConnection::send(ENetPacket *packet)
{
    send(m_peer, packet);
}

void NetworkConnection::broadcast(ENetPacket *packet, ENetPeer *peer)
{
    ZoneScoped;

    // PacketInfo p(packet, true, peer);
    // m_packet_queue.enqueue(p);

    ENetAction action{};
    action.type = ENetAction::TYPE_PACKET;
    action.packet.packet = packet;
    action.packet.broadcast = true;
    action.packet.filter = peer;
    m_action_queue.enqueue(action);

    // for (const auto& [key, value] : m_clients)
    // {
    //     if (peer == value.peer())
    //         continue;
    //     std::lock_guard<std::mutex> lock(m_host_mutex);
    //     enet_peer_send(value.peer(), 0, packet);
    // }
}

void NetworkConnection::disconnect(ENetPeer *peer)
{
    ZoneScoped;

    ENetAction action{};
    action.type = ENetAction::TYPE_DISCONNECT;
    action.disconnect.peer = peer;
    m_action_queue.enqueue(action);

    // std::lock_guard<std::mutex> lock(m_host_mutex);
    // enet_peer_disconnect(peer, 0);
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
    ZoneScoped;

    ENetEvent event;
    while (m_event_queue.try_dequeue(event))
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
            // close();
        };
        break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            Client client("", 0, event.peer);
            m_packet_handler(m_packet_handler_user, *this, event.packet, client);
            // enet_packet_destroy(event.packet);
        }
        break;
        default:
            break;
        }
    }
}

void NetworkConnection::tick_server()
{
    ZoneScoped;

    ENetEvent event;
    while (m_event_queue.try_dequeue(event))
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
        {
            char address_buf[32];
            enet_address_get_host_ip(&event.peer->address, address_buf, sizeof(address_buf));

            Client client(address_buf, event.peer->address.port, event.peer);
            m_clients[event.peer] = client;

            m_connect_handler(m_disconnect_handler_user, *this, client);

            info("Client connected from {}:{}", client.ip(), client.port());
        }
        break;
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            m_disconnect_handler(m_disconnect_handler_user, *this, m_clients[event.peer]);

            const Client& client = m_clients[event.peer];
            info("Client disconnected from {}:{}", client.ip(), client.port());

            m_clients.erase(event.peer);
        };
        break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            const Client& client = m_clients[event.peer];
            m_packet_handler(m_packet_handler_user, *this, event.packet, client);
            // enet_packet_destroy(event.packet);
        }
        break;
        default:
            break;
        }
    }
}

void NetworkConnection::close()
{
    m_worker_state.store(false);
    if (m_worker_thread.joinable())
        m_worker_thread.join();

    switch (m_state)
    {
    case ConnectionState::Host:
    {
        for (const auto& [peer, client] : m_clients)
            enet_peer_disconnect(peer, 0);
        // std::lock_guard<std::mutex> guard(m_host_mutex);
        enet_host_destroy(m_host);
        m_host = nullptr;
        m_clients.clear();
    };
    break;
    case ConnectionState::Connected:
    case ConnectionState::Connection:
    {
        if (m_state == ConnectionState::Connected)
            enet_peer_disconnect(m_peer, 0);
        // std::lock_guard<std::mutex> guard(m_host_mutex);
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
    break;
    default:
        break;
    }

    // Reset the state connection state to idle.
    m_state = ConnectionState::Idle;
}

void NetworkConnection::worker()
{
    TracySetThreadName("Network Worker");

    while (m_worker_state.load())
    {
        ZoneScoped;

        ENetAction action{};
        while (m_worker_state.load() && m_action_queue.try_dequeue(action))
        {
            switch (action.type)
            {
            case ENetAction::TYPE_PACKET:
            {
                if (action.packet.broadcast)
                {
                    bool success = false;
                    for (const auto& [key, value] : m_clients)
                    {
                        if (action.packet.filter == value.peer())
                            continue;
                        if (enet_peer_send(value.peer(), 0, action.packet.packet) == 0)
                            success = true;
                    }
                    if (!success)
                        enet_packet_destroy(action.packet.packet);
                }
                else
                {
                    if (enet_peer_send(action.packet.filter, 0, action.packet.packet) < 0)
                        enet_packet_destroy(action.packet.packet);
                }
            }
            break;
            case ENetAction::TYPE_DISCONNECT:
            {
                enet_peer_disconnect(action.disconnect.peer, 0);
            };
            break;
            }
        }

        // std::lock_guard<std::mutex> lock(m_host_mutex);
        ENetEvent event{};
        while (m_worker_state.load() && enet_host_service(m_host, &event, 0) > 0)
        {
            m_event_queue.enqueue(event);
        }
    }
}
