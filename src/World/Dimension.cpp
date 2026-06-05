#include "World/Dimension.hpp"

#include "Block/Block.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include <limits>

Option<Ref<Chunk>> Dimension::get_chunk(int64_t x, int64_t z) const
{
    return m_chunks.get(ChunkPos(x, z));
}

bool Dimension::has_chunk(int64_t x, int64_t z) const
{
    return m_chunks.contains(ChunkPos(x, z));
}

Result<void> Dimension::add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    TRY(m_chunks_to_flush.put(ChunkPos(x, z), chunk));
    return Result<void>();
}

void Dimension::remove_chunk(int64_t x, int64_t z)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    EXPECT(m_chunks_to_remove.append(ChunkPos(x, z)));
}

Result<void> Dimension::add_entity(Ref<Entity> entity)
{
    return m_entities_to_add.append(entity);
}

void Dimension::remove_entity(Ref<Entity> entity)
{
    EXPECT(m_entities_to_remove.append(entity));
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
        for (int64_t y = std::max(min_y, int64_t(0)); y <= std::min(max_y, Chunk::height - 1); y++)
        {
            for (int64_t z = min_z; z <= max_z; z++)
            {
                if (!has_solid_block(x, y, z))
                    continue;

                AABB block_box = AABB(-glm::vec3(0.5), glm::vec3(0.5)).translate(glm::vec3(x, y, z));
                EXPECT(boxes.append(block_box)); // TODO: change to `TRY`
            }
        }
    }

    return boxes;
}

Vector<Ref<Entity>> Dimension::cast_box(const AABB& box) const
{
    Vector<Ref<Entity>> entities;

    for (Ref<Entity>& entity : m_entities)
    {
        if (entity->get_aabb().translate(entity->get_position()).intersect(box))
            EXPECT(entities.append(entity));
    }

    return entities;
}

BlockState Dimension::get_block(int64_t x, int64_t y, int64_t z) const
{
    if (y < 0 || y >= Chunk::height)
        return BlockState();

    int64_t chunk_x = chunk_index(x);
    int64_t chunk_z = chunk_index(z);

    const auto chunk_maybe = get_chunk(chunk_x, chunk_z);
    if (!chunk_maybe)
        return BlockState();

    Ref<Chunk> chunk = chunk_maybe.get();
    int64_t local_x = local_coords(x);
    int64_t local_z = local_coords(z);

    return chunk->get_block(local_x, y, local_z);
}

void Dimension::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    if (y < 0 || y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(x);
    int64_t chunk_z = chunk_index(z);

    Option<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.get();
    int64_t local_x = local_coords(x);
    int64_t local_z = local_coords(z);

    chunk->set_block(local_x, y, local_z, state);

    if (local_x == 0)
    {
        chunk_value = get_chunk(chunk_x - 1, chunk_z);
        if (chunk_value.has_value())
            EXPECT(chunk_value.get()->build_simple_mesh(y / 16));
    }
    else if (local_x == 15)
    {
        chunk_value = get_chunk(chunk_x + 1, chunk_z);
        if (chunk_value.has_value())
            EXPECT(chunk_value.get()->build_simple_mesh(y / 16));
    }
    if (local_z == 0)
    {
        chunk_value = get_chunk(chunk_x, chunk_z - 1);
        if (chunk_value.has_value())
            EXPECT(chunk_value.get()->build_simple_mesh(y / 16));
    }
    else if (local_z == 15)
    {
        chunk_value = get_chunk(chunk_x, chunk_z + 1);
        if (chunk_value.has_value())
            EXPECT(chunk_value.get()->build_simple_mesh(y / 16));
    }
}

void Dimension::set_tag(glm::i64vec3 pos, const StringView& name, Variant v)
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    Option<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.get();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    chunk->set_tag({local_x, pos.y, local_z}, name, v);
}

void Dimension::remove_tag(glm::i64vec3 pos, const StringView& name)
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    Option<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.get();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    chunk->remove_tag({local_x, pos.y, local_z}, name);
}

Option<Variant> Dimension::get_tag(glm::i64vec3 pos, const StringView& name) const
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return None;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    Option<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return None;
    }

    Ref<Chunk> chunk = chunk_value.get();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    return chunk->get_tag({local_x, pos.y, local_z}, name);
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    return !get_block(x, y, z).is_air();
}

Result<void> Dimension::rebuild_neighbor_chunks_water(int64_t cx, int64_t cz, size_t slice_index)
{
    const Array<glm::i64vec2, 4> array{
        glm::i64vec2(1, 0),
        glm::i64vec2(-1, 0),
        glm::i64vec2(0, 1),
        glm::i64vec2(0, -1),
    };

    for (const glm::i64vec2 dir : array)
    {
        Option<Ref<Chunk>> chunk = get_chunk(cx + dir.x, cz + dir.y);
        if (chunk.has_value())
        {
            if (slice_index == std::numeric_limits<size_t>::max())
            {
                for (size_t i = 0; i < Chunk::slice_count; i++)
                    TRY(chunk.get()->build_water_mesh(i));
            }
            else
            {
                TRY(chunk.get()->build_water_mesh(slice_index));
            }
        }
    }

    return Result<void>();
}

Result<Ref<Chunk>> Dimension::generate_chunk(int64_t cx, int64_t cz)
{
    ZoneScoped;

    Ref<Chunk> chunk = TRY(newref<Chunk>(this, cx, cz));
    memset(chunk->get_biomes(), 0, sizeof(Biome) * Chunk::block_count);

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = 0; y < Chunk::height; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                int64_t gx = cx * 16 + x;
                int64_t gz = cz * 16 + z;

                BlockState state = generate_block(gx, y, gz, chunk);
                chunk->get_blocks()[Chunk::linearize(x, y, z)] = state;

                // there is at least one non-empty block.
                if (!state.is_air())
                    chunk->get_slices()[y / Chunk::width].empty = false;
            }
        }
    }

    for (size_t i = 0; i < Chunk::slice_count; i++)
    {
        TRY(chunk->build_simple_mesh(i));
        TRY(chunk->build_water_mesh(i));
    }

    return chunk;
}

BlockState Dimension::generate_block(int64_t x, int64_t y, int64_t z, Ref<Chunk>& chunk)
{
    BlockState state;
    for (size_t index = 0; index < m_generation_passes.size(); index++)
    {
        Ref<GenerationPass>& pass = m_generation_passes.get_unchecked(index);
        state = pass->generate_block(x, y, z, chunk);
    }
    return state;
}
