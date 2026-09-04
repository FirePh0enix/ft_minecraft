#include "World/Dimension.hpp"

#include "AABB.hpp"
#include "Block/Block.hpp"
#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"
#include "Variant.hpp"
#include "World/Chunk.hpp"
#include "World/Gen.hpp"
#include "World/World.hpp"

#include <mutex>

void GenScheduler::terrain_pass(ChunkPos middle)
{
    for (int64_t x = -(m_chunk_distance + m_gen_distance); x <= m_chunk_distance + m_gen_distance; x++)
        for (int64_t z = -(m_chunk_distance + m_gen_distance); z <= m_chunk_distance + m_gen_distance; z++)
        {
            const ChunkPos pos(x + middle.x, z + middle.z);

            if (m_dimension.m_preloaded_chunks.contains(pos) || m_dimension.m_pregen_loading_queue.contains(pos))
                continue;

            m_dimension.m_pregen_loading_queue.insert(pos);

            std::shared_ptr<PreLoadedChunk> chunk = std::make_shared<PreLoadedChunk>();
            chunk->pos = pos;

            Engine::get().get_thread_pool().submit([this, pos, chunk](std::stop_token token)
                                                   { terrain_and_struct_chunk(token, pos, chunk); });
        }

    for (auto iter = m_dimension.m_preloaded_chunks.begin(); iter != m_dimension.m_preloaded_chunks.end();)
    {
        const ChunkPos pos = iter->first;
        if (std::abs(pos.x - middle.x) > (m_chunk_distance + m_gen_distance + 1) || std::abs(pos.z - middle.z) > (m_chunk_distance + m_gen_distance + 1))
            iter = m_dimension.m_preloaded_chunks.erase(iter);
        else
            ++iter;
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
    // Structures can overlap multiple chunks, so finish the complete
    // pre-generation pass before realizing any chunk blocks.
    if (!m_dimension.m_pregen_loading_queue.empty())
        return;

    for (int64_t x = -m_chunk_distance; x <= m_chunk_distance; x++)
        for (int64_t z = -m_chunk_distance; z <= m_chunk_distance; z++)
        {
            const ChunkPos pos(x + middle.x, z + middle.z);

            if (m_dimension.m_chunks.contains(pos) || m_dimension.m_chunks_loading_queue.contains(pos))
                continue;

            std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(&m_dimension, pos.x, pos.z);
            std::shared_ptr<PreLoadedChunk> preload_chunk = m_dimension.m_preloaded_chunks.at(pos);
            m_dimension.m_chunks_loading_queue.insert(pos);
            Engine::get().get_thread_pool().submit([this, pos, chunk, preload_chunk](std::stop_token st)
                                                   { realize_chunk(st, pos, chunk, preload_chunk); });
        }

    std::vector<std::shared_ptr<Chunk>> chunks;
    for (const auto& [pos, chunk] : m_dimension.m_chunks)
    {
        if (std::abs(middle.x - pos.x) > m_chunk_distance || std::abs(middle.z - pos.z) > m_chunk_distance)
            chunks.push_back(chunk);
    }
    for (std::shared_ptr<Chunk> chunk : chunks)
    {
        const ChunkPos pos = chunk->pos();
        m_dimension.m_chunks.erase(pos);
        m_dimension.m_chunks_rebuild_queue.erase(pos);
        m_dimension.m_chunks_rebuild_pending.erase(pos);
        m_dimension.queue_unload_chunk(chunk);
    }
}

void GenScheduler::terrain_and_struct_chunk(std::stop_token token, ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk)
{
    if (token.stop_requested())
        return;
    m_dimension.m_gen->preload(pos.x, pos.z, chunk);

    if (token.stop_requested())
        return;
    m_dimension.m_gen->structure_pass(pos.x, pos.z, chunk, m_dimension);

    m_dimension.m_pregen_chunks_lockless.enqueue(chunk);
}

void GenScheduler::realize_chunk(std::stop_token token, ChunkPos pos, std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> pregen_chunk)
{
    std::string path = std::format("{}saves/{}/DIM0/{}${}/blocks.dat", Filesystem::get_data_directory(), m_dimension.m_world->get_name(), pos.x, pos.z);

    if (false && !Engine::get().is_save_disabled() && Filesystem::exists(path))
    {
        std::vector<char> data;
        // TODO: how to handle errors from loading chunks ?
        File file = EXPECT(Filesystem::open_file(path));
        EXPECT(file.reader().read_to_buffer(data));
        file.close();

        std::vector<uint8_t> blocks_data;
        auto blocks_result = ZLib::inflate_with_cancellation(token, std::as_bytes(std::span(data)), blocks_data);
        if (!blocks_result)
        {
            if (token.stop_requested())
                return;
            EXPECT(blocks_result);
        }

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
            auto tags_result = ZLib::inflate_with_cancellation(token, std::as_bytes(std::span(tags_compressed_data)), tags_data);
            if (!tags_result)
            {
                if (token.stop_requested())
                    return;
                EXPECT(tags_result);
            }

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
        EXPECT(m_dimension.m_world->save_chunk(token, chunk, m_dimension.m_id));
    }

    m_dimension.m_chunks_lockless.enqueue(chunk);
}

Dimension::Dimension(int id)
    : m_id(id), m_scheduler(*this)
{
}

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

    auto chunk = chunk_maybe.value();
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

    auto chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    auto chunk = chunk_value.value();
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

    auto chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    auto chunk = chunk_value.value();
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

    auto chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    auto chunk = chunk_value.value();
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

    auto chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
        return std::nullopt;

    auto chunk = chunk_value.value();
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
    return block->has_collision();
}

void Dimension::rebuild(std::stop_token token, std::shared_ptr<Chunk> chunk, const std::map<ChunkPos, std::shared_ptr<Chunk>>& nchunks, size_t slice_index, size_t slice_count)
{
    MeshRebuildResult results[16];

    for (size_t i = slice_index; i < slice_index + slice_count; i++)
    {
        if (token.stop_requested())
            return;

        results[i].pos = chunk->pos();
        results[i].chunk = chunk;
        results[i].slice_index = i;

        Result<std::shared_ptr<Mesh>> opaque_mesh = chunk->build_opaque_mesh(i, nchunks);
        if (opaque_mesh.has_error())
            return;
        Result<std::shared_ptr<Mesh>> water_mesh = chunk->build_water_mesh(i, nchunks);
        if (water_mesh.has_error())
            return;

        results[i].opaque_mesh = opaque_mesh.value();
        results[i].water_mesh = water_mesh.value();
    }

    m_mesh_queue_lockless.enqueue_bulk(std::begin(results) + slice_index, slice_count);
}

void Dimension::queue_rebuild(ChunkPos pos, size_t slice_index, size_t slice_count)
{
    if (!m_chunks.contains(pos))
        return;
    if (m_chunks_rebuild_queue.contains(pos))
    {
        m_chunks_rebuild_pending.insert(pos);
        return;
    }

    auto chunk = m_chunks[pos];
    std::map<ChunkPos, std::shared_ptr<Chunk>> nchunks;

    const std::array<ChunkPos, 4> positions{
        ChunkPos(pos.x + 1, pos.z),
        ChunkPos(pos.x - 1, pos.z),
        ChunkPos(pos.x, pos.z + 1),
        ChunkPos(pos.x, pos.z - 1),
    };
    for (ChunkPos p : positions)
    {
        auto chunk_opt = get_chunk(p.x, p.z);
        if (!chunk_opt.has_value())
        {
            continue;
        }
        nchunks[p] = chunk_opt.value();
    }

    m_chunks_rebuild_queue.insert(pos);
    Engine::get().get_mesh_thread_pool().submit([this, chunk, nchunks, slice_index, slice_count](std::stop_token token)
                                                { rebuild(token, chunk, nchunks, slice_index, slice_count); });
}

void Dimension::remove_preload(ChunkPos pos)
{
    m_preloaded_chunks.erase(pos);
}

void Dimension::unload_chunk(std::stop_token token, std::shared_ptr<Chunk> chunk)
{
    if (token.stop_requested())
        return;
    if (!Engine::get().is_save_disabled())
    {
        Result<void> result = m_world->save_chunk({}, chunk, m_id);
        (void)result;
    }

    // TODO: only push the ChunkPos.
    m_chunks_unload_queue.enqueue(chunk);
}

void Dimension::queue_unload_chunk(std::shared_ptr<Chunk> chunk)
{
    Engine::get().get_thread_pool().submit([this, chunk](std::stop_token token)
                                           { unload_chunk(token, chunk); });
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

void Dimension::write_tags(Writer& writer, std::shared_ptr<Chunk> chunk)
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

void Dimension::read_tags(Reader& reader, std::shared_ptr<Chunk> chunk)
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
