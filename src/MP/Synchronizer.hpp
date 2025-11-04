#pragma once

#include "Core/Class.hpp"
#include "MP/Peer.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Entity.hpp"
#include "Scene/System.hpp"

struct MPProperty
{
    EntityPath path;
    ClassHashCode component;
    Property property;

    Variant cached_value;
};

class MPSynchronizer : public Component
{
    CLASS(MPSynchronizer, Component);

public:
    MPSynchronizer();

    void add_property(const EntityPath& path, ClassHashCode component, const Property& property);

    /**
     * Synchronize properties with connected peers.
     */
    static void sync_properties_with_peers(const Query<Many<MPSynchronizer>, Many<MPPeer>>& query);

private:
    std::vector<MPProperty> m_properties;
};
