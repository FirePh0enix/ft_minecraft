#include "World/Dimension.hpp"

#include "Block/Block.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/World.hpp"

std::optional<std::shared_ptr<Chunk>> Dimension::get_chunk(int64_t x, int64_t z) const
{
    auto iter = m_chunks.find(ChunkPos(x, z));
    if (iter != m_chunks.end())
        return iter->second;
    return std::nullopt;
}

bool Dimension::has_chunk(int64_t x, int64_t z) const
{
    return m_chunks.contains(ChunkPos(x, z));
}

void Dimension::add_chunk(int64_t x, int64_t z, const std::shared_ptr<Chunk>& chunk)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    m_chunks_to_flush[ChunkPos(x, z)] = chunk;
}

void Dimension::remove_chunk(int64_t x, int64_t z)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    m_chunks_to_remove.push_back(ChunkPos(x, z));
}

void Dimension::add_entity(std::shared_ptr<Entity> entity)
{
    m_entities_to_add.push_back(entity);
}

void Dimension::remove_entity(std::shared_ptr<Entity> entity)
{
    m_entities_to_remove.push_back(entity);
}

void Dimension::remove_entity(EntityId id)
{
    std::shared_ptr<Entity> entity = get_entity(id);
    m_entities_to_remove.push_back(entity);
}

std::shared_ptr<Entity> Dimension::get_entity(EntityId id) const
{
    for (const auto& entity : m_entities)
    {
        if (entity->id() == id)
            return entity;
    }
    return nullptr;
}

std::vector<AABB> Dimension::get_boxes_that_may_collide(const AABB& box) const
{
    std::vector<AABB> boxes;
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
                boxes.push_back(block_box);
            }
        }
    }

    return boxes;
}

std::vector<std::shared_ptr<Entity>> Dimension::cast_box(const AABB& box) const
{
    std::vector<std::shared_ptr<Entity>> entities;

    for (const std::shared_ptr<Entity>& entity : m_entities)
    {
        if (entity->get_aabb().translate(entity->get_position()).intersect(box))
            entities.push_back(entity);
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

    std::shared_ptr<Chunk> chunk = chunk_maybe.value();
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

    std::optional<std::shared_ptr<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    std::shared_ptr<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(x);
    int64_t local_z = local_coords(z);

    chunk->set_block(local_x, y, local_z, state);
}

void Dimension::set_tag(glm::i64vec3 pos, std::string_view name, Variant v)
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    std::optional<std::shared_ptr<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    std::shared_ptr<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    chunk->set_tag({local_x, pos.y, local_z}, name, v);
}

void Dimension::remove_tag(glm::i64vec3 pos, std::string_view name)
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    std::optional<std::shared_ptr<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    std::shared_ptr<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    chunk->remove_tag({local_x, pos.y, local_z}, name);
}

std::optional<Variant> Dimension::get_tag(glm::i64vec3 pos, std::string_view name) const
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return std::nullopt;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    std::optional<std::shared_ptr<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
        return std::nullopt;

    std::shared_ptr<Chunk> chunk = chunk_value.value();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    return chunk->get_tag({local_x, pos.y, local_z}, name);
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    return !get_block(x, y, z).is_air();
}

Result<std::shared_ptr<Chunk>> Dimension::generate_chunk(int64_t cx, int64_t cz)
{
    std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(this, cx, cz);
    memset((void *)chunk->get_blocks(), 0, sizeof(BlockState) * Chunk::block_count);

    for (int i = 0; i < 16 * 16; i++)
        chunk->get_biomes()[i] = Biome::Plain;

    m_gen->generate_chunk(chunk);
    return chunk;
}

void Dimension::rebuild(ChunkPos pos)
{
    std::shared_ptr<Chunk> chunk;
    std::map<ChunkPos, std::shared_ptr<Chunk>> nchunks;

    {
        std::lock_guard<std::mutex> lock(mutex());

        std::optional<std::shared_ptr<Chunk>> chunk_opt = get_chunk(pos.x, pos.z);
        if (!chunk_opt.has_value())
        {
            return;
        }

        chunk = chunk_opt.value();

        const std::array<ChunkPos, 4> positions{
            ChunkPos(pos.x + 1, pos.z),
            ChunkPos(pos.x - 1, pos.z),
            ChunkPos(pos.x, pos.z + 1),
            ChunkPos(pos.x, pos.z - 1),
        };
        for (ChunkPos p : positions)
        {
            chunk_opt = get_chunk(p.x, p.z);
            if (!chunk_opt.has_value())
            {
                continue;
            }
            nchunks[p] = chunk_opt.value();
        }
    }

    for (size_t i = 0; i < Chunk::slice_count; i++)
    {
        EXPECT(chunk->build_simple_mesh(i, nchunks));
        EXPECT(chunk->build_water_mesh(i, nchunks));
    }
}

void Dimension::queue_rebuild(ChunkPos pos)
{
    std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
    if (m_chunk_rebuild_queue.contains(pos))
    {
        return;
    }
    m_chunk_rebuild_queue.insert(pos);

    Engine::get().get_thread_pool().async([this, pos]
                                          {
                                            rebuild(pos);
                                            std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
                                            m_chunk_rebuild_queue.erase(pos); });
}

void Dimension::update_sun(glm::mat4 matrix)
{
    m_sun_frustum = Frustum(matrix);
}
