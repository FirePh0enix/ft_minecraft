#include "Scene/Scene.hpp"
#include "Scene/Components/Visual.hpp"

Scene::Scene()
{
}

void Scene::encode_draw_calls(RenderPassEncoder& encoder)
{
    for (const auto& entity : m_entites)
    {
        if (Ref<VisualComponent> visual_comp = entity->get_component<VisualComponent>())
        {
            visual_comp->encode_draw_calls(encoder, *m_active_camera);
        }
    }
}

void Scene::tick()
{
    ZoneScopedN("Scene::tick");

    for (auto& entity : m_entites)
    {
        entity->tick();
    }
}
