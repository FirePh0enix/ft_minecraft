#include "MP/Synchronizer.hpp"

MPSynchronizer::MPSynchronizer()
{
}

void MPSynchronizer::add_property(const Ref<Object>& instance, size_t offset, size_t size)
{
    m_properties.push_back(MPProperty{.instance = instance, .offset = offset, .size = size});
}

void sync_properties_with_peers(const Query<Many<MPSynchronizer>, Many<MPPeer>>& query)
{
    for (const auto& results : query.get<0>().results())
    {
    }
}
