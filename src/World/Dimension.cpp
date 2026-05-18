#include "World/Dimension.hpp"
#include "Profiler.hpp"
#include "World/Block.hpp"
#include "World/Chunk.hpp"
#include <algorithm>

std::optional<Ref<Chunk>> Dimension::get_chunk(int64_t x, int64_t z) const
{
    auto iter = m_chunks.find(ChunkPos(x, z));
    if (iter == m_chunks.end())
        return std::nullopt;
    return iter->second;
}

bool Dimension::has_chunk(int64_t x, int64_t z) const
{
    return m_chunks.contains(ChunkPos(x, z));
}

Result<void> Dimension::add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
{
    m_chunks[ChunkPos(x, z)] = chunk;
    return Result<void>();
}

void Dimension::remove_chunk(int64_t x, int64_t z)
{
    auto iter = m_chunks.find(ChunkPos(x, z));
    if (iter != m_chunks.end())
        m_chunks.erase(iter);
}

Result<void> Dimension::add_entity(Ref<Entity> entity)
{
    return m_entities.append(entity);
}

void Dimension::remove_entity(EntityId id)
{
    if (id.is_valid())
    {
        m_entities.remove_if(
            [&](const Ref<Entity>& entity)
            {
                return entity->id() == id;
            });
    }
}

Ref<Entity> Dimension::get_entity(EntityId id) const
{
    for (const auto& entity : m_entities)
    {
        if (entity->id() == id)
            return entity;
    }
    return nullptr;
}

Vector<AABB> Dimension::get_boxes_that_may_collide(const AABB& box) const
{
    Vector<AABB> boxes;
    int64_t size = 3;

    glm::i64vec3 pos = box.center();

    int64_t min_x = pos.x - size, max_x = pos.x + size;
    int64_t min_y = pos.y - size, max_y = pos.y + size;
    int64_t min_z = pos.z - size, max_z = pos.z + size;

    for (int64_t x = min_x; x <= max_x; x++)
    {
        // ! On mac we need to use 0ll and not 0l.
        for (int64_t y = std::max(min_y, 0ll); y <= std::min(max_y, Chunk::height - 1); y++)
        {
            for (int64_t z = min_z; z <= max_z; z++)
            {
                int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
                int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

                const auto chunk_maybe = get_chunk(chunk_x, chunk_z);
                if (!chunk_maybe)
                    continue;

                int64_t local_x = x >= 0 ? (x % 16) : (16 + x % 16);
                int64_t local_z = z >= 0 ? (z % 16) : (16 + z % 16);

                Ref<Chunk> chunk = chunk_maybe.value();
                if (chunk->get_block(local_x, y, local_z).is_air())
                    continue;

                AABB block_box = AABB(-glm::vec3(0.5), glm::vec3(0.5)).translate(glm::vec3(x, y, z));
                EXPECT(boxes.append(block_box)); // TODO: change to `TRY`
            }
        }
    }

    return boxes;
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    if (y < 0 || y >= Chunk::height)
        return false;

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

    Ref<Chunk> chunk = TRY(newref<Chunk>(cx, cz));
    memset(chunk->get_biomes(), 0, sizeof(Biome) * Chunk::block_count_with_overlap);

    for (int64_t x = 0; x < Chunk::width_with_overlap; x++)
    {
        for (int64_t y = 0; y < Chunk::height; y++)
        {
            for (int64_t z = 0; z < Chunk::width_with_overlap; z++)
            {
                int64_t gx = cx * 16 + (x - 1);
                int64_t gz = cz * 16 + (z - 1);

                size_t block_index = Chunk::linearize_with_overlap(x, y, z);
                BlockState state = generate_block(gx, y, gz);
                chunk->get_blocks()[block_index] = state;

                // there is at least one non-empty block.
                if (!state.is_air())
                    chunk->get_slices()[y / Chunk::width].empty = false;
            }
        }
    }

    for (size_t i = 0; i < Chunk::slice_count; i++)
        TRY(chunk->build_simple_mesh(i));

    return chunk;
}

BlockState Dimension::generate_block(int64_t x, int64_t y, int64_t z)
{
    BlockState state;
    for (size_t index = 0; index < m_generation_passes.size(); index++)
    {
        Ref<GenerationPass>& pass = m_generation_passes.get_unchecked(index);
        state = pass->generate_block(x, y, z);
    }
    return state;
}
