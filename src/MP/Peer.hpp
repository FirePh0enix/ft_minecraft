#pragma once

#include "Core/Class.hpp"
#include "Scene/Components/Component.hpp"

#include <enet/enet.h>

class MPPeer : public Component
{
    CLASS(MPPeer, Component);

public:
    MPPeer();

    ALWAYS_INLINE bool is_authority() const { return m_authority; }
    ALWAYS_INLINE void set_authority(bool authority) { m_authority = authority; }

private:
    bool m_authority;
    uint32_t m_id;

    // This can be either a host or a client.
    ENetHost *m_enet_host;
};
