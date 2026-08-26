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

void GenScheduler::terrain_pass(ChunkPos middle)
{
    std::set<ChunkPos> chunks;

    for (int64_t x = -(m_chunk_distance + m_gen_distance); x <= m_chunk_distance + m_gen_distance; x++)
        for (int64_t z = -(m_chunk_distance + m_gen_distance); z <= m_chunk_distance + m_gen_distance; z++)
        {
            const ChunkPos pos(x + middle.x, z + middle.z);

            if (m_dimension.has_pregen_chunk(x + middle.x, z + middle.z))
                continue;

            {
                std::lock_guard<std::mutex> lock(m_pregen_queue_mutex);
                if (m_pregen_queue.contains(pos))
                    continue;
                m_pregen_queue.insert(pos);
            }

            chunks.insert(pos);
        }

    if (chunks.size() > 0)
    {
        m_pregen_count.fetch_add(chunks.size());

        for (const ChunkPos& pos : chunks)
        {
            Engine::get().get_thread_pool().async([this, pos]()
                                                  { terrain_and_struct_chunk(pos); });
        }
    }

    // println("{}", m_pregen_count.load());

    // std::lock_guard<std::mutex> lock(m_dimension.m_preload_mutex);
    for (auto iter : m_dimension.m_preloaded_chunks)
    {
        const ChunkPos pos = iter.first;
        if (std::abs(pos.x - middle.x) > m_chunk_distance + m_gen_distance || std::abs(pos.z - middle.z) > m_chunk_distance + m_gen_distance)
            m_pregen_unload_queue.push_back(pos);
    }
    for (const ChunkPos& pos : m_pregen_unload_queue)
    {
        m_dimension.m_preloaded_chunks.erase(pos);
    }
}

// static ChunkPos pop_near(std::vector<ChunkLoadWithDistance>& elements)
// {
//     float min_distance = elements[0].distance;
//     size_t min_index = 0;
//     for (size_t i = 1; i < elements.size(); i++)
//     {
//         if (elements[i].distance > min_distance)
//         {
//             min_distance = elements[i].distance;
//             min_index = i;
//         }
//     }
//     ChunkPos pos = elements[min_index].pos;
//     elements.erase(elements.begin() + (ssize_t)min_index);
//     return pos;
// }

void GenScheduler::chunk_pass(ChunkPos middle)
{
    if (m_pregen_count.load() > 0)
    {
        return;
    }

    for (int64_t x = -m_chunk_distance; x <= m_chunk_distance; x++)
        for (int64_t z = -m_chunk_distance; z <= m_chunk_distance; z++)
        {
            const ChunkPos pos(x + middle.x, z + middle.z);

            if (m_dimension.m_chunks.contains(pos) || m_dimension.m_chunks_loading_queue.contains(pos))
                continue;

            m_dimension.m_chunks_loading_queue.insert(pos);

            std::shared_ptr<PreLoadedChunk> preload_chunk = m_dimension.m_preloaded_chunks[pos];
            Engine::get().get_thread_pool().async([this, pos, preload_chunk]()
                                                  { realize_chunk(pos, preload_chunk); });
        }

    for (const auto& [pos, chunk] : m_dimension.m_chunks)
    {
        if (std::abs(middle.x - pos.x) > m_chunk_distance || std::abs(middle.z - pos.z) > m_chunk_distance)
            m_dimension.queue_unload_chunk(chunk);
    }
}

void GenScheduler::terrain_and_struct_chunk(ChunkPos pos)
{
    std::shared_ptr<PreLoadedChunk> chunk = std::make_shared<PreLoadedChunk>();
    chunk->pos = pos;
    m_dimension.m_gen->preload(pos.x, pos.z, chunk);
    m_dimension.m_gen->structure_pass(pos.x, pos.z, chunk, m_dimension);

    m_dimension.m_pregen_chunks_lockless.push_wait(chunk);
}

void GenScheduler::realize_chunk(ChunkPos pos, std::shared_ptr<PreLoadedChunk> pregen_chunk)
{
    std::string path = std::format("{}saves/{}/DIM0/{}${}/blocks.dat", Filesystem::get_data_directory(), m_dimension.m_world->get_name(), pos.x, pos.z);
    Chunk *chunk = m_dimension.alloc_chunk(pos.x, pos.z).value();

    if (!Engine::get().is_save_disabled() && Filesystem::exists(path))
    {
        std::vector<char> data;
        // TODO: how to handle errors from loading chunks ?
        File file = EXPECT(Filesystem::open_file(path));
        EXPECT(file.reader().read_to_buffer(data));
        file.close();

        std::vector<uint8_t> blocks_data;
        EXPECT(ZLib::inflate(std::as_bytes(std::span(data)), blocks_data));

        assert(blocks_data.size() == sizeof(BlockState) * Chunk::block_count);
        memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

        std::string path = std::format("{}saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_dimension.m_world->get_name(), pos.x, pos.z);
        if (Filesystem::exists(path))
        {
            File file = EXPECT(Filesystem::open_file(path));
            std::vector<char> tags_compressed_data;
            EXPECT(file.reader().read_to_buffer(tags_compressed_data));
            file.close();

            std::vector<uint8_t> tags_data;
            EXPECT(ZLib::inflate(std::as_bytes(std::span(tags_compressed_data)), tags_data));

            BufferReader reader(tags_data.data(), tags_data.size());
            Dimension::read_tags(reader, chunk);
        }
    }
    else
    {
        memset((void *)chunk->get_blocks(), 0, sizeof(BlockState) * Chunk::block_count);

        for (int i = 0; i < 16 * 16; i++)
            chunk->get_biomes()[i] = Biome::Plain;

        m_dimension.m_gen->generate_chunk(chunk, pregen_chunk, m_dimension);

        // Save the initial version of the chunk.
        EXPECT(m_dimension.m_world->save_chunk(chunk, m_dimension.m_id));
    }

    m_dimension.m_chunks_lockless.push_wait(chunk);
}

Dimension::Dimension(int id)
    : m_id(id), m_scheduler(*this)
{
    m_chunk_pool = new Chunk[4096]();
    m_chunk_pool_state = new std::atomic_bool[4096]();
}

std::optional<Chunk *> Dimension::get_chunk(int64_t x, int64_t z) const
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

bool Dimension::has_pregen_chunk(int64_t x, int64_t z) const
{
    return m_preloaded_chunks.contains(ChunkPos(x, z));
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

void Dimension::load(int64_t x, int64_t y, int64_t z, int64_t distance)
{
    (void)distance;
    const glm::i64vec3 player_pos(x, y, z);
    const int64_t player_cx = int64_t(player_pos.x / 16);
    const int64_t player_cz = int64_t(player_pos.z / 16);
    const ChunkPos player_cpos(player_cx, player_cz);

    m_scheduler.terrain_pass(player_cpos);
    m_scheduler.chunk_pass(player_cpos);
}

std::vector<AABBd> Dimension::get_boxes_that_may_collide(const AABBd& box) const
{
    std::vector<AABBd> boxes;
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

                AABBd block_box = AABBd(-glm::dvec3(0.5), glm::dvec3(0.5)).translate(glm::dvec3(x, y, z));
                boxes.push_back(block_box);
            }
        }
    }

    return boxes;
}

std::vector<std::shared_ptr<Entity>> Dimension::cast_box(const AABBd& box) const
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

    Chunk *chunk = chunk_maybe.value();
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

    std::optional<Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Chunk *chunk = chunk_value.value();
    int64_t local_x = local_coords(x);
    int64_t local_z = local_coords(z);

    chunk->set_block(local_x, y, local_z, state);
}

void Dimension::set_tag(glm::i64vec3 pos, std::string_view name, std::string v)
{
    if (pos.y < 0 || pos.y >= Chunk::height)
        return;

    int64_t chunk_x = chunk_index(pos.x);
    int64_t chunk_z = chunk_index(pos.z);

    std::optional<Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Chunk *chunk = chunk_value.value();
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

    std::optional<Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Chunk *chunk = chunk_value.value();
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

    std::optional<Chunk *> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
        return std::nullopt;

    Chunk *chunk = chunk_value.value();
    int64_t local_x = local_coords(pos.x);
    int64_t local_z = local_coords(pos.z);

    return chunk->get_tag({local_x, pos.y, local_z}, name);
}

bool Dimension::has_solid_block(int64_t x, int64_t y, int64_t z) const
{
    BlockState state = get_block(x, y, z);
    if (state.is_air())
        return false;
    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
    if (block == nullptr)
        return true;
    return block->is_solid();
}

void Dimension::rebuild(Chunk *chunk, const std::map<ChunkPos, Chunk *>& nchunks, size_t slice_index, size_t slice_count)
{
    MeshRebuildResult results[16];

    for (size_t i = slice_index; i < slice_count; i++)
    {
        results[i].pos = chunk->pos();
        results[i].slice_index = i;
        results[i].mesh = EXPECT(chunk->build_simple_mesh(i, nchunks));
        results[i].water_mesh = EXPECT(chunk->build_water_mesh(i, nchunks));
    }

    for (size_t i = slice_index; i < slice_count; i++)
    {
        while (!m_mesh_queue_lockless.push(results[i]))
            ;
    }
}

void Dimension::queue_rebuild(ChunkPos pos, size_t slice_index, size_t slice_count)
{
    if (!m_chunks.contains(pos))
        return;

    // TODO: remove this mutex as well.
    {
        std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
        if (m_chunk_rebuild_queue.contains(pos))
            return;
        m_chunk_rebuild_queue.insert(pos);
    }

    Chunk *chunk = m_chunks[pos];
    std::map<ChunkPos, Chunk *> nchunks;

    const std::array<ChunkPos, 4> positions{
        ChunkPos(pos.x + 1, pos.z),
        ChunkPos(pos.x - 1, pos.z),
        ChunkPos(pos.x, pos.z + 1),
        ChunkPos(pos.x, pos.z - 1),
    };
    for (ChunkPos p : positions)
    {
        std::optional<Chunk *> chunk_opt = get_chunk(p.x, p.z);
        if (!chunk_opt.has_value())
        {
            continue;
        }
        nchunks[p] = chunk_opt.value();
    }

    Engine::get().get_thread_pool().async([this, chunk, nchunks, slice_index, slice_count]
                                          {
                                            rebuild(chunk, nchunks, slice_index, slice_count);
                                            std::lock_guard<std::mutex> lock(m_chunk_rebuild_mutex);
                                            m_chunk_rebuild_queue.erase(chunk->pos()); });
}

void Dimension::remove_preload(ChunkPos pos)
{
    m_preloaded_chunks.erase(pos);
}

void Dimension::unload_chunk(Chunk *chunk)
{
    if (!Engine::get().is_save_disabled())
    {
        Result<void> result = m_world->save_chunk(chunk, m_id);
        (void)result;
    }

    m_chunks_unload_queue.push_wait(chunk);
}

void Dimension::queue_unload_chunk(Chunk *chunk)
{
    Engine::get().get_thread_pool().async([this, chunk]
                                          { unload_chunk(chunk); });
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

std::optional<Chunk *> Dimension::alloc_chunk(int64_t x, int64_t z)
{
    for (size_t i = 0; i < 4096; i++)
    {
        bool b = false;
        if (m_chunk_pool_state[i].compare_exchange_strong(b, true))
        {
            new (&m_chunk_pool[i]) Chunk(this, x, z);
            return std::make_optional(&m_chunk_pool[i]);
        }
    }
    return std::nullopt;
}

void Dimension::free_chunk(Chunk *chunk)
{
    size_t index = chunk - m_chunk_pool;
    bool b = true;
    m_chunk_pool_state[index].compare_exchange_strong(b, false);
}

void Dimension::write_tags(Writer& writer, const Chunk *chunk)
{
    std::map<int64_t, std::map<std::string, Variant>> tags;
    for (const auto& [key, value] : chunk->m_tags)
    {
        std::map<std::string, Variant> tags2;
        for (const auto& [key2, value2] : value)
            tags2[key2] = value2;
        tags[key] = tags2;
    }
    EXPECT(writer.write_variant(Variant(tags)));
}

void Dimension::read_tags(Reader& reader, Chunk *chunk)
{
    std::optional<Variant> variant = EXPECT(reader.read_variant());
    if (variant.has_value())
    {
        std::map<int64_t, std::map<std::string, Variant>> tags = variant.value().to_map<int64_t, std::map<std::string, Variant>>();
        for (const auto& [key, value] : tags)
        {
            stdext::string_map<Variant> btags;
            for (const auto& [key2, value2] : value)
                btags[key2] = value2;
            chunk->m_tags[key] = btags;
        }
    }
}
