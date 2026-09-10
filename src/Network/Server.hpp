#pragma once

#include "World/World.hpp"

class Server
{
public:
    virtual ~Server() {}

    virtual void start() = 0;
    virtual void tick() = 0;

    virtual void send_message(const std::string& name) = 0;

    virtual void route_packet(ENetPacket *packet)
    {
        (void)packet;
        enet_packet_destroy(packet);
    }

    std::shared_ptr<World> get_world() const { return m_world; }
    std::shared_ptr<Player> get_player() const { return m_player; }

protected:
    std::shared_ptr<World> m_world;
    std::shared_ptr<Player> m_player;
    int64_t m_ticks_since_start_of_day = 0;
};
