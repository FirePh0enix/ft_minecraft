#pragma once

#include "Core/Class.hpp"

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;
typedef struct _ENetAddress ENetAddress;

class Scene;

class MPManager : public Object
{
    CLASS(MPManager, Object);

    static MPManager& get()
    {
        static MPManager singleton;
        return singleton;
    }

    ~MPManager();

    void host(uint16_t port, size_t maximum_clients);

    /**
     *
     */
    void connect_to(const ENetAddress& address);

private:
    MPManager();

    ENetHost *m_host = nullptr;
    ENetPeer *m_foreign_peer = nullptr;
};

struct MultiplayerPlugin
{
    static void setup(Scene *scene);
};
