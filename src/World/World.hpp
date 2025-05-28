#pragma once

#include "Camera.hpp"
#include "Render/Driver.hpp"
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

class World
{
public:
    World();

    void set_render_distance(uint32_t distance);
    void encode_draw_calls(RenderGraph& graph, Mesh *mesh, Material *material, Camera& camera);

    void generate_flat(BlockState state);

private:
    std::vector<Chunk> m_chunks;
    std::vector<Ref<Buffer>> m_buffers;
    uint32_t m_distance = 0;
};
