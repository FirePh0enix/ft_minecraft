#include "Scene/Scene.hpp"

Scene::Scene()
{
}

void Scene::encode_draw_calls(RenderGraph& graph)
{
    for (const auto& entity : m_entites)
    {
        Ref<VisualComponent> visual_comp = entity->get_component<VisualComponent>(0);

        // if (Ref<VisualComponent> visual_comp = entity->get_component<VisualComponent>(0))
        // {
        //     std::println("dwkjjd");
        visual_comp->encode_draw_calls(graph, *m_active_camera);
        // }
    }
}
