#pragma once

#include "Camera.hpp"
#include "Render/Driver.hpp"
#include "Scene/Entity.hpp"
#include "World/Chunk.hpp"

struct BlockInstanceData
{
    glm::vec3 position;
    glm::vec3 textures0;
    glm::vec3 textures1;
    uint8_t visibility;
    uint8_t gradient = 0;
    uint8_t gradient_type = 0;
    uint8_t pad = 0;
};

class World : public VisualComponent
{
    CLASS(World, VisualComponent);

public:
    World(Ref<Mesh> mesh, Ref<Material> material);

    void set_render_distance(uint32_t distance);
    void generate_flat(BlockState state);

    virtual void encode_draw_calls(RenderGraph& graph, Camera& camera) const override;

private:
    std::vector<Chunk> m_chunks;
    std::vector<Ref<Buffer>> m_buffers;
    uint32_t m_distance = 0;

    Ref<Mesh> m_mesh;
    Ref<Material> m_material;
};
