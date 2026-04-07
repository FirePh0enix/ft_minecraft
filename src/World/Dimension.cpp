#include "World/Dimension.hpp"
#include "Profiler.hpp"

Vector<AABB> Dimension::get_boxes_that_may_collide(const AABB& box) const
{
    Vector<AABB> boxes;
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
                int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

                const auto chunk_maybe = get_chunk(chunk_x, chunk_z);
                if (!chunk_maybe)
                    continue;

                int64_t local_x = x >= 0 ? (x % 16) : (16 + x % 16);
                int64_t local_y = y >= 0 ? (y % 16) : (16 + y % 16);
                int64_t local_z = z >= 0 ? (z % 16) : (16 + z % 16);

                Ref<Chunk> chunk = chunk_maybe.value();
                if (chunk->get_block(local_x, local_y, local_z).is_air())
                    continue;

                AABB block_box(glm::vec3(x, y, z) + glm::vec3(0.5), glm::vec3(0.5, 0.5, 0.5));
                EXPECT(boxes.append(block_box)); // TODO: change to `TRY`
            }
        }
    }

    return boxes;
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    const auto chunk_maybe = get_chunk(chunk_x, chunk_z);
    if (!chunk_maybe)
        return false;

    Ref<Chunk> chunk = chunk_maybe.value();
    int64_t local_x = x >= 0 ? (x % 16) : (16 + x % 16);
    int64_t local_z = z >= 0 ? (z % 16) : (16 + z % 16);

    return !chunk->get_block(local_x, y, local_z).is_air();
}

Result<Ref<Chunk>> Dimension::generate_chunk(int64_t cx, int64_t cz)
{
    ZoneScoped;

    Ref<Chunk> chunk = newref<Chunk>(cx, cz, m_world);
    if (!chunk)
        return Error(ErrorKind::OutOfMemory);

    for (size_t i = 0; i < Chunk::block_count_with_overlap; i++)
        chunk->get_blocks()[i] = BlockState();

    memset(chunk->get_biomes(), 0, sizeof(Biome) * Chunk::block_count_with_overlap);

    for (size_t index = 0; index < m_generation_passes.size(); index++)
    {
        Ref<GenerationPass>& pass = m_generation_passes.get_unchecked(index);
        for (int64_t x = 0; x < Chunk::width_with_overlap; x++)
        {
            for (int64_t y = 0; y < Chunk::height; y++)
            {
                for (int64_t z = 0; z < Chunk::width_with_overlap; z++)
                {
                    int64_t gx = cx * 16 + (x - 1);
                    int64_t gz = cz * 16 + (z - 1);

                    size_t block_index = Chunk::linearize_with_overlap(x, y, z);

                    pass->generate(gx, y, gz, block_index, chunk);
                    BlockState state = chunk->get_blocks()[block_index];

                    // there is at least one non-empty block.
                    if (!state.is_air())
                        chunk->get_slices()[y / Chunk::width].empty = false;
                }
            }
        }
    }

    for (size_t i = 0; i < Chunk::slice_count; i++)
        TRY(chunk->build_simple_mesh(i));

    return chunk;
}
