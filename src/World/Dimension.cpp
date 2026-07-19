#include "World/Dimension.hpp"

#include "Block/Block.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/World.hpp"
#include "Engine.hpp"

#include <limits>

Option<Ref<Chunk>> Dimension::get_chunk(int64_t x, int64_t z) const
{
    return m_chunks.get(ChunkPos(x, z));
}

bool Dimension::has_chunk(int64_t x, int64_t z) const
{
    return m_chunks.contains(ChunkPos(x, z));
}

void Dimension::add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    m_chunks_to_flush.put(ChunkPos(x, z), chunk);
}

void Dimension::remove_chunk(int64_t x, int64_t z)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    m_chunks_to_remove.append(ChunkPos(x, z));
}

void Dimension::add_entity(Ref<Entity> entity)
{
    m_entities_to_add.append(entity);
}

void Dimension::remove_entity(Ref<Entity> entity)
{
    m_entities_to_remove.append(entity);
}

void Dimension::remove_entity(EntityId id)
{
    Ref<Entity> entity = get_entity(id);
    m_entities_to_remove.append(entity);
}

Ref<Entity> Dimension::get_entity(EntityId id) const
{
    for (const auto& entity : m_entities) {
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
                boxes.append(block_box);
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
            entities.append(entity);
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

    Ref<Chunk> chunk = chunk_maybe.value();
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

    Ref<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(x);
    int64_t local_z = local_coords(z);

    chunk->set_block(local_x, y, local_z, state);
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

    Ref<Chunk> chunk = chunk_value.value();
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

    Ref<Chunk> chunk = chunk_value.value();
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

    Ref<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    return chunk->get_tag({local_x, pos.y, local_z}, name);
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    return !get_block(x, y, z).is_air();
}

Result<Ref<Chunk>> Dimension::generate_chunk(int64_t cx, int64_t cz)
{
    ZoneScoped;

    Ref<Chunk> chunk = newref<Chunk>(this, cx, cz);
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
            }
        }
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

void Dimension::rebuild(ChunkPos pos)
{
    Ref<Chunk> chunk;
    Map<ChunkPos, Ref<Chunk>> nchunks;

    {
	std::lock_guard<std::mutex> lock(mutex());

	Option<Ref<Chunk>> chunk_opt = get_chunk(pos.x, pos.z);
	if (!chunk_opt.has_value()) {
	    return;
	}

	chunk = chunk_opt.value();

	const Array<ChunkPos, 4> positions{
	    ChunkPos(pos.x + 1, pos.z),
	    ChunkPos(pos.x - 1, pos.z),
	    ChunkPos(pos.x, pos.z + 1),
	    ChunkPos(pos.x, pos.z - 1),
	};
	for (ChunkPos p : positions) {
	    chunk_opt = get_chunk(p.x, p.z);
	    if (!chunk_opt.has_value()) {
		continue;
	    }
	    nchunks.put(p, chunk_opt.value());
	}
    }

    for (size_t i = 0; i < Chunk::slice_count; i++) {
	EXPECT(chunk->build_simple_mesh(i, nchunks));
	EXPECT(chunk->build_water_mesh(i, nchunks));
    }
}

void Dimension::queue_rebuild(ChunkPos pos)
{
    std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
    if (m_chunk_rebuild_queue.contains(pos)) {
	return;
    }
    m_chunk_rebuild_queue.put(pos);
    
    Engine::get().get_thread_pool().async([this, pos] {
	rebuild(pos);

	std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
	// FIXME: spurious heap-buffer-overflow when ASan is enabled.
	m_chunk_rebuild_queue.erase(pos);
    });
}
