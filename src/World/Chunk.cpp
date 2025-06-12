#include "World/Chunk.hpp"
#include "World/World.hpp"

void Chunk::update_instance_buffer(Ref<Buffer> buffer)
{
    std::vector<BlockInstanceData> instances;
    instances.reserve(Chunk::block_count);

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t z = 0; z < Chunk::width; z++)
        {
            for (int64_t y = 0; y < Chunk::height; y++)
            {
                BlockState state = get_block(x, y, z);
                if (state.id == 0)
                    continue;

                int64_t gx = m_x * 16 + x;
                int64_t gz = m_z * 16 + z;

                instances.push_back(BlockInstanceData{.position = glm::vec3((float)gx, (float)y, (float)gz), .textures = glm::uvec3(), .visibility = 0xff});
            }
        }
    }

    buffer->update(Span(instances).as_bytes());
    m_block_count = instances.size();
}
