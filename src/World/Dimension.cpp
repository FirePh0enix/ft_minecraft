#include "World/Dimension.hpp"

#include "AABB.hpp"
#include "Block/Block.hpp"
#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Gen.hpp"
#include "World/World.hpp"

#include <mutex>

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

void Dimension::add_chunk(const std::shared_ptr<Chunk>& chunk)
{
    std::lock_guard<std::mutex> g(m_chunk_mutex);
    m_chunks_to_flush[chunk->pos()] = chunk;
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

static ChunkPos pop_near(std::vector<ChunkLoadWithDistance>& elements)
{
    float min_distance = elements[0].distance;
    size_t min_index = 0;
    for (size_t i = 1; i < elements.size(); i++)
    {
        if (elements[i].distance > min_distance)
        {
            min_distance = elements[i].distance;
            min_index = i;
        }
    }
    ChunkPos pos = elements[min_index].pos;
    elements.erase(elements.begin() + (ssize_t)min_index);
    return pos;
}

void Dimension::load(int64_t x, int64_t y, int64_t z, int64_t distance)
{
    const glm::i64vec3 player_pos(x, y, z);
    int64_t player_cx = int64_t(player_pos.x / 16);
    int64_t player_cz = int64_t(player_pos.z / 16);

    const int64_t preload_distance = std::max(distance * 2, 32l);

    for (int64_t cx = -preload_distance * 2; cx <= preload_distance * 2; cx++)
        for (int64_t cz = -preload_distance * 2; cz <= preload_distance * 2; cz++)
        {
            int64_t x = player_cx + cx;
            int64_t z = player_cz + cz;
            ChunkPos pos(x, z);

            // FIXME: Lock a mutex to do a simple check is not ideal
            {
                std::lock_guard<std::mutex> lock(m_preload_mutex);
                if (m_preloaded_chunks.contains(pos))
                    continue;
            }

            queue_preload_chunk(pos);
        }

    m_load_buffer.resize(0);
    for (int64_t cx = -distance; cx <= distance; cx++)
        for (int64_t cz = -distance; cz <= distance; cz++)
        {
            int64_t x = player_cx + cx;
            int64_t z = player_cz + cz;
            ChunkPos pos(x, z);

            if (has_chunk(x, z))
                continue;

            // TODO: needs biome data only when generating the biome, not when loading it.
            //       find a way to only generate this data if its not saved to the disk.
            {
                std::lock_guard<std::mutex> lock(m_preload_mutex);
                if (!m_preloaded_chunks.contains(pos))
                    continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_chunk_mutex);
                if (m_chunks_to_flush.contains(pos))
                    continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_chunk_loading_mutex);
                if (m_chunk_loading_queue.contains(pos))
                    continue;

                m_chunk_loading_queue.insert(pos);
            }

            m_load_buffer.push_back(ChunkLoadWithDistance(pos, glm::distance2(glm::vec2(player_pos.x, player_pos.z), glm::vec2((float)pos.x * 16.0f + 8.0f, (float)pos.z * 16.0f + 8.0f))));
        }

    const size_t size = m_load_buffer.size();
    for (size_t i = 0; i < size; i++)
    {
        ChunkPos pos = pop_near(m_load_buffer);
        queue_load_chunk(pos);
    }

    std::lock_guard<std::mutex> lock(m_chunk_mutex);
    for (const auto& [pos, chunk] : m_chunks)
    {
        if (pos.x >= player_cx - distance && pos.x <= player_cx + distance && pos.z >= player_cz - distance && pos.z <= player_cz + distance)
            continue;
        queue_unload_chunk(pos);
    }

    // std::lock_guard<std::mutex> lock2(m_preload_mutex);
    // for (const auto& [pos, chunk] : m_preloaded_chunks)
    // {
    //     if (pos.x >= player_cx - preload_distance * 2 && pos.x <= player_cx + preload_distance * 2 && pos.z >= player_cz - preload_distance * 2 && pos.z <= player_cz + preload_distance * 2)
    //         continue;
    //     remove_preload(pos);
    // }
}

std::vector<AABBf> Dimension::get_boxes_that_may_collide(const AABBf& box) const
{
    std::vector<AABBf> boxes;
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

                AABBf block_box = AABBf(-glm::vec3(0.5), glm::vec3(0.5)).translate(glm::vec3(x, y, z));
                boxes.push_back(block_box);
            }
        }
    }

    return boxes;
}

std::vector<std::shared_ptr<Entity>> Dimension::cast_box(const AABBf& box) const
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

    std::shared_ptr<PreLoadedChunk> preloaded_chunk;
    {
        std::lock_guard<std::mutex> lock(m_preload_mutex);
        auto iter = m_preloaded_chunks.find(ChunkPos(cx, cz));
        if (iter == m_preloaded_chunks.end())
            return Error(ErrorKind::Unknown);
        preloaded_chunk = iter->second;
    }

    m_gen->generate_chunk(chunk, preloaded_chunk, *this);
    return chunk;
}

void Dimension::rebuild(ChunkPos pos, size_t slice_index, size_t slice_count)
{
    std::shared_ptr<Chunk> chunk;
    std::map<ChunkPos, std::shared_ptr<Chunk>> nchunks;

    {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);

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

    for (size_t i = slice_index; i < slice_count; i++)
    {
        EXPECT(chunk->build_simple_mesh(i, nchunks));
        EXPECT(chunk->build_water_mesh(i, nchunks));
    }
}

void Dimension::queue_rebuild(ChunkPos pos, size_t slice_index, size_t slice_count)
{
    std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
    if (m_chunk_rebuild_queue.contains(pos))
    {
        return;
    }
    m_chunk_rebuild_queue.insert(pos);

    Engine::get().get_thread_pool().async([this, pos, slice_index, slice_count]
                                          {
                                            rebuild(pos, slice_index, slice_count);
                                            std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
                                            m_chunk_rebuild_queue.erase(pos); });
}

void Dimension::preload_chunk(ChunkPos pos)
{
    std::shared_ptr<PreLoadedChunk> chunk = std::make_shared<PreLoadedChunk>();
    m_gen->preload(pos.x, pos.z, chunk);
    m_gen->structure_pass(pos.x, pos.z, chunk, *this);

    std::lock_guard<std::mutex> lock(m_preload_mutex);
    m_preloaded_chunks[pos] = chunk;
}

void Dimension::queue_preload_chunk(ChunkPos pos)
{
    Engine::get().get_thread_pool().async([this, pos]
                                          { preload_chunk(pos); });
}

void Dimension::remove_preload(ChunkPos pos)
{
    m_preloaded_chunks.erase(pos);
}

void Dimension::load_chunk(ChunkPos pos)
{
    std::shared_ptr<Chunk> chunk;

    std::string path = std::format("{}saves/{}/DIM0/{}${}/blocks.dat", Filesystem::get_data_directory(), m_world->get_name(), pos.x, pos.z);
    if (!Engine::get().is_save_disabled() && Filesystem::exists(path))
    {
        chunk = std::make_shared<Chunk>(this, pos.x, pos.z);

        std::vector<char> data;
        // TODO: how to handle errors from loading chunks ?
        File file = EXPECT(Filesystem::open_file(path));
        EXPECT(file.reader().read_to_buffer(data));
        file.close();

        std::vector<uint8_t> blocks_data;
        EXPECT(ZLib::inflate(std::as_bytes(std::span(data)), blocks_data));

        assert(blocks_data.size() == sizeof(BlockState) * Chunk::block_count);
        memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

        std::string path = std::format("{}saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_world->get_name(), pos.x, pos.z);
        if (Filesystem::exists(path))
        {
            File file = EXPECT(Filesystem::open_file(path));
            std::vector<char> tags_compressed_data;
            EXPECT(file.reader().read_to_buffer(tags_compressed_data));
            file.close();

            std::vector<uint8_t> tags_data;
            EXPECT(ZLib::inflate(std::as_bytes(std::span(tags_compressed_data)), tags_data));

            BufferReader reader(tags_data.data(), tags_data.size());
            read_tags(reader, chunk);
        }
    }
    else
    {
        Result<std::shared_ptr<Chunk>> result = generate_chunk(pos.x, pos.z);
        if (result.has_error())
            return;
        chunk = result.value();

        EXPECT(m_world->save_chunk(chunk));
    }

    add_chunk(chunk);

    {
        std::lock_guard<std::mutex> lock(m_chunk_loading_mutex);
        m_chunk_loading_queue.erase(pos);
    }
}

void Dimension::queue_load_chunk(ChunkPos pos)
{
    Engine::get().get_thread_pool().async([this, pos]
                                          { load_chunk(pos); });
}

void Dimension::unload_chunk(ChunkPos pos)
{
    remove_chunk(pos.x, pos.z);
}

void Dimension::queue_unload_chunk(ChunkPos pos)
{
    Engine::get().get_thread_pool().async([this, pos]
                                          { unload_chunk(pos); });
}

void Dimension::update_sun(glm::mat4 matrix)
{
    m_sun_frustum = Frustum(matrix);
}

void Dimension::place_structure(glm::i64vec3 pos, BlockState *blocks, int64_t w, int64_t h, int64_t l)
{
    std::lock_guard<std::mutex> lock(m_structures_mutex);
    m_structures_queue.push_back(StructureGen(pos, blocks, w, h, l));
}

void Dimension::get_structures_overlap(ChunkPos pos, std::vector<StructureGen>& structures)
{
    std::lock_guard<std::mutex> lock(m_structures_mutex);

    const AABBi chunk_box(glm::i64vec3(pos.x * 16, 0, pos.z * 16),
                          glm::i64vec3(pos.x * 16 + 16, 256, pos.z * 16 + 16));

    for (const StructureGen& structure : m_structures_queue)
    {
        const AABBi box(structure.pos,
                        structure.pos + glm::i64vec3(structure.w, structure.h, structure.l));

        if (box.intersect(chunk_box))
            structures.push_back(structure);
    }
}

void Dimension::write_tags(Writer& writer, const std::shared_ptr<Chunk>& chunk)
{
    std::map<int64_t, std::map<std::string, Variant>> tags;
    for (const auto& [key, value] : chunk->m_tags)
    {
        std::map<std::string, Variant> tags2;
        for (const auto& [key2, value2] : value.tags)
            tags2[key2] = value2;
        tags[key] = tags2;
    }
    EXPECT(writer.write_variant(Variant(tags)));
}

void Dimension::read_tags(Reader& reader, std::shared_ptr<Chunk>& chunk)
{
    std::optional<Variant> variant = EXPECT(reader.read_variant());
    if (variant.has_value())
    {
        std::map<int64_t, std::map<std::string, Variant>> tags = variant.value().to_map<int64_t, std::map<std::string, Variant>>();
        for (const auto& [key, value] : tags)
        {
            BlockTags btags;
            for (const auto& [key2, value2] : value)
                btags.tags[key2] = value2;
            chunk->m_tags[key] = btags;
        }
    }
}
