#include "MP/Manager.hpp"

#include <enet/enet.h>

MPManager::MPManager()
{
}

MPManager::~MPManager()
{
    if (m_host)
        enet_host_destroy(m_host);
}

void MPManager::host(uint16_t port, size_t maximum_clients)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, maximum_clients, 2, 0, 0);
    ASSERT_V(m_host != nullptr, "Failed to create server host on address 0.0.0.0:{}", port);
}

void MPManager::connect_to(const ENetAddress& address)
{
    m_host = enet_host_create(nullptr, 1, 2, 0, 0);
    ASSERT(m_host != nullptr, "Failed to create client host on address");

    m_foreign_peer = enet_host_connect(m_host, &address, 2, 0);
    ASSERT_V(m_host != nullptr, "Failed to connect to {}:{}", address.host, address.port);
}

void MultiplayerPlugin::setup(Scene *scene)
{
}
