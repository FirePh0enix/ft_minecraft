#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "MP/Peer.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/System.hpp"

struct MPProperty
{
    Ref<Object> instance;
    size_t offset;
    size_t size;
};

class MPSynchronizer : public Component
{
    CLASS(MPSynchronizer, Component);

public:
    MPSynchronizer();

    void add_property(const Ref<Object>& instance, size_t offset, size_t size);

private:
    std::vector<MPProperty> m_properties;
};

/**
 * Synchronize properties with connected peers.
 */
void sync_properties_with_peers(const Query<Many<MPSynchronizer>, Many<MPPeer>>& query);
