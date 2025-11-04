#include "MP/Synchronizer.hpp"
#include "Scene/Scene.hpp"

MPSynchronizer::MPSynchronizer()
{
}

void MPSynchronizer::add_property(const EntityPath& path, ClassHashCode component, const Property& property)
{
    m_properties.push_back(MPProperty{.path = path, .component = component, .property = property, .cached_value = {nullptr}});
}

void MPSynchronizer::sync_properties_with_peers(const Query<Many<MPSynchronizer>, Many<MPPeer>>& query)
{
    for (const auto& result : query.get<0>().results())
    {
        const Ref<MPSynchronizer>& sync = result.get<MPSynchronizer>();

        for (const MPProperty& property : sync->m_properties)
        {
            const Ref<Entity>& entity = query.scene()->get_entity(property.path);
            if (entity.is_null()) // Maybe add a warning ?
                continue;

            const Ref<Component>& comp = entity->get_component(property.component);
        }
    }
}
