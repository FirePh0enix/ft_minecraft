#include "World/Dimension.hpp"

std::vector<AABB> Dimension::get_overlapping_boxes(const AABB& box) const
{
    std::vector<AABB> boxes;

    int64_t chunk_x = box.center.x >= 0 ? ((int64_t)box.center.x / 16) : ((int64_t)box.center.x / 16 - 1);
    int64_t chunk_y = box.center.y >= 0 ? ((int64_t)box.center.y / 16) : ((int64_t)box.center.y / 16 - 1);
    int64_t chunk_z = box.center.z >= 0 ? ((int64_t)box.center.z / 16) : ((int64_t)box.center.z / 16 - 1);

    const glm::i64vec3 centeri = box.center;

    for (int64_t cx = chunk_x - 1; cx <= chunk_x + 1; cx++)
        for (int64_t cy = chunk_y - 1; cy <= chunk_y + 1; cy++)
            for (int64_t cz = chunk_z - 1; cz <= chunk_z + 1; cz++)
            {
                const auto chunk_maybe = get_chunk(cx, cy, cz);

                // Chunk is not loaded
                if (!chunk_maybe)
                    continue;

                Ref<Chunk> chunk = chunk_maybe.value();
                const ChunkPos& chunk_pos = chunk->pos();

                for (int64_t x = 0; x < Chunk::width; x++)
                    for (int64_t y = 0; y < Chunk::width; y++)
                        for (int64_t z = 0; z < Chunk::width; z++)
                        {
                            const glm::i64vec3 pos = glm::i64vec3(x, y, z) + glm::i64vec3(
                                                                                 chunk_pos.x * Chunk::width,
                                                                                 chunk_pos.y * Chunk::width,
                                                                                 chunk_pos.z * Chunk::width);

                            if (chunk->get_block(x, y, z).is_air())
                                continue;

                            AABB block_box(pos, glm::vec3(0.5, 0.5, 0.5));
                            if (box.intersect(block_box))
                            {
                                // println("[ {} {} {} ] vs [ {} {} {} ]", pos.x, pos.y, pos.z, box.center.x, box.center.y, box.center.z);
                                boxes.push_back(box);
                            }
                        }
            }
    return boxes;
}
