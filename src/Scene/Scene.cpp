#include "Scene/Scene.hpp"
#include "Scene/Components/Visual.hpp"

Scene::Scene()
{
}

void Scene::encode_draw_calls(RenderPassEncoder& encoder)
{
    for (const auto& entity : m_entities)
    {
        if (Ref<VisualComponent> visual_comp = entity->get_component<VisualComponent>())
        {
            visual_comp->encode_draw_calls(encoder, *m_active_camera);
        }
    }
}

void Scene::tick(float delta)
{
    m_physics_space.step(delta);
    // for (Ref<Entity>& entity : m_entities)
    //     entity->tick(delta);
}
