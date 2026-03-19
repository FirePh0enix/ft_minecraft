#include "World/Dimension.hpp"

std::vector<AABB> Dimension::get_boxes_that_may_collide(const AABB& box) const
{
    std::vector<AABB> boxes;

    const glm::i64vec3 centeri = box.center;

    int64_t min_x = (int64_t)box.center.x - 2, max_x = (int64_t)box.center.x + 2;
    int64_t min_y = (int64_t)box.center.y - 2, max_y = (int64_t)box.center.y + 2;
    int64_t min_z = (int64_t)box.center.z - 2, max_z = (int64_t)box.center.z + 2;
    for (int64_t x = min_x; x <= max_x; x++)
    {
        for (int64_t y = min_y; y <= max_y; y++)
        {
            for (int64_t z = min_z; z <= max_z; z++)
            {
                int64_t chunk_x = x >= 0 ? ((int64_t)x / 16) : ((int64_t)x / 16 - 1);
                int64_t chunk_y = y >= 0 ? ((int64_t)y / 16) : ((int64_t)y / 16 - 1);
                int64_t chunk_z = z >= 0 ? ((int64_t)z / 16) : ((int64_t)z / 16 - 1);

                const auto chunk_maybe = get_chunk(chunk_x, chunk_y, chunk_z);
                if (!chunk_maybe)
                    continue;

                Ref<Chunk> chunk = chunk_maybe.value();
                if (chunk->get_block(x, y, z).is_air())
                    continue;

                AABB block_box(glm::vec3(x, y, z) + glm::vec3(0.5), glm::vec3(0.5, 0.5, 0.5));
                boxes.push_back(block_box);
            }
        }
    }

    return boxes;
}
