#include "World/Dimension.hpp"

std::vector<AABB> Dimension::get_boxes_that_may_collide(const AABB& box) const
{
    std::vector<AABB> boxes;
    int64_t size = 3;

    int64_t min_x = (int64_t)box.center.x - size, max_x = (int64_t)box.center.x + size;
    int64_t min_y = (int64_t)box.center.y - size, max_y = (int64_t)box.center.y + size;
    int64_t min_z = (int64_t)box.center.z - size, max_z = (int64_t)box.center.z + size;
    for (int64_t x = min_x; x <= max_x; x++)
    {
        for (int64_t y = min_y; y <= max_y; y++)
        {
            for (int64_t z = min_z; z <= max_z; z++)
            {
                int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
                int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
                int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

                const auto chunk_maybe = get_chunk(chunk_x, chunk_y, chunk_z);
                if (!chunk_maybe)
                    continue;

                int64_t local_x = x >= 0 ? (x % 16) : (16 + x % 16);
                int64_t local_y = y >= 0 ? (y % 16) : (16 + y % 16);
                int64_t local_z = z >= 0 ? (z % 16) : (16 + z % 16);

                Ref<Chunk> chunk = chunk_maybe.value();
                if (chunk->get_block(local_x, local_y, local_z).is_air())
                    continue;

                AABB block_box(glm::vec3(x, y, z) + glm::vec3(0.5), glm::vec3(0.5, 0.5, 0.5));
                boxes.push_back(block_box);
            }
        }
    }

    return boxes;
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    const auto chunk_maybe = get_chunk(chunk_x, chunk_y, chunk_z);
    if (!chunk_maybe)
        return false;

    Ref<Chunk> chunk = chunk_maybe.value();
    int64_t local_x = x >= 0 ? (x % 16) : (16 + x % 16);
    int64_t local_y = y >= 0 ? (y % 16) : (16 + y % 16);
    int64_t local_z = z >= 0 ? (z % 16) : (16 + z % 16);

    return !chunk->get_block(local_x, local_y, local_z).is_air();
}
