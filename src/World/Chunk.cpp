#include "World/Chunk.hpp"
#include "World/World.hpp"

Chunk Chunk::flat(ssize_t x, ssize_t z, BlockState state, uint32_t height)
{
    Chunk chunk(x, z);

    for (size_t x = 0; x < 16; x++)
    {
        for (size_t z = 0; z < 16; z++)
        {
            for (size_t y = 0; y < height; y++)
            {
                chunk.set_block(x, y, z, state);
            }
        }
    }

    return chunk;
}

void Chunk::update_instance_buffer(Ref<Buffer> buffer)
{
    std::vector<BlockInstanceData> instances;
    instances.reserve(Chunk::block_count);

    for (size_t x = 0; x < Chunk::width; x++)
    {
        for (size_t z = 0; z < Chunk::width; z++)
        {
            for (size_t y = 0; y < Chunk::height; y++)
            {
                BlockState state = get_block(x, y, z);
                if (state.id == 0)
                    continue;

                ssize_t gx = m_x * 16 + (ssize_t)x;
                ssize_t gz = m_z * 16 + (ssize_t)z;

                instances.push_back(BlockInstanceData{.position = glm::vec3((float)gx, (float)y, (float)gz), .textures0 = glm::vec3(), .textures1 = glm::vec3(), .visibility = 0xff});
            }
        }
    }

    Span<BlockInstanceData> span = instances;
    buffer->update(span.as_bytes());

    m_block_count = instances.size();
}
