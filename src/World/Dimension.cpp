#include "World/Dimension.hpp"

#include "AABB.hpp"
#include "Block/Block.hpp"
#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Entity/Player.hpp"
#include "Profiler.hpp"
#include "Variant.hpp"
#include "World/Chunk.hpp"
#include "World/Gen.hpp"
#include "World/World.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>

void GenScheduler::update_order(ChunkPos direction)
{
    const double length = std::hypot(double(direction.x), double(direction.z));
    const double dx = length > 0 ? direction.x / length : 0;
    const double dz = length > 0 ? direction.z / length : 0;
    const auto fill = [&](auto& order, int64_t radius)
    {
        order.clear();
        for (int64_t x = -radius; x <= radius; ++x)
            for (int64_t z = -radius; z <= radius; ++z)
                order.emplace_back(x, z);
        const auto score = [&](ChunkPos p) {
            // Keep the immediate neighborhood first; bias farther work toward
            // the next four chunks along the travel direction.
            const double distance = double(p.x*p.x + p.z*p.z);
            return distance <= 4 ? distance - 100.0
                : distance - 8.0 * (p.x*dx + p.z*dz);
        };
        std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) { return score(a) < score(b); });
    };
    fill(m_terrain_order, m_chunk_distance + m_gen_distance);
    fill(m_chunk_order, m_chunk_distance);
}

bool GenScheduler::wanted(ChunkPos pos, int64_t radius) const
{
    const auto origins = std::atomic_load(&m_active_origins);
    if (!origins)
        return true; // Standalone scheduling before the first tick.
    for (const auto origin : *origins)
        if (std::abs(pos.x-origin.x) <= radius && std::abs(pos.z-origin.z) <= radius)
            return true;
    return false;
}

bool GenScheduler::dependencies_ready(ChunkPos pos) const
{
    for (int64_t x = -m_gen_distance; x <= m_gen_distance; ++x)
        for (int64_t z = -m_gen_distance; z <= m_gen_distance; ++z)
            if (!m_dimension.m_preloaded_chunks.contains({pos.x+x, pos.z+z}))
                return false;
    return true;
}

bool GenScheduler::terrain_pass(ChunkPos middle)
{
    if (m_terrain_order.empty())
        update_order();
    // Published preloads include their completed cross-border structure pass.
    bool ready = true;
    size_t scheduled = 0;

    for (const auto offset : m_terrain_order)
    {
        const ChunkPos pos(offset.x + middle.x, offset.z + middle.z);

        if (m_dimension.m_preloaded_chunks.contains(pos))
            continue;

        // Work that is queued or newly scheduled still makes this origin
        // incomplete, but must not block unrelated origins.
        ready = false;
        if (m_pregen_loading_queue.contains(pos))
            continue;

        // Bound new work even when an entire region becomes needed.
        if (scheduled == 128 || m_pregen_loading_queue.size() >= 64)
            return false;
        ++scheduled;
        m_pregen_loading_queue.insert(pos);

        Engine::get().get_thread_pool().submit([this, pos](std::stop_token token)
        {
            if (token.stop_requested())
                return;
            if (!wanted(pos, m_chunk_distance + m_gen_distance + 1))
            {
                m_cancelled_terrain.enqueue(pos);
                return;
            }
            auto chunk = std::make_shared<PreLoadedChunk>();
            chunk->pos = pos;
            terrain_and_struct_chunk(token, pos, chunk);
        });
    }
    return ready;
}

bool GenScheduler::chunk_pass(ChunkPos middle)
{
    m_generation_started = true;
    if (m_chunk_order.empty())
        update_order();
    size_t scheduled = 0;
    bool ready = true;

    for (const auto offset : m_chunk_order)
    {
        const ChunkPos pos(offset.x + middle.x, offset.z + middle.z);

        if (m_dimension.m_chunks.contains(pos) || m_chunks_loading_queue.contains(pos))
            continue;

        if (!dependencies_ready(pos))
        {
            ready = false;
            continue;
        }
        if (scheduled == 32 || m_chunks_loading_queue.size() >= 16)
            return false;
        ++scheduled;
        std::shared_ptr<PreLoadedChunk> preload_chunk = m_dimension.m_preloaded_chunks.at(pos);
        m_chunks_loading_queue.insert(pos);
        // Allocate/zero the 65,536-block array on a worker, not in a
        // main-thread burst for every chunk in the newly scheduled region.
        Engine::get().get_thread_pool().submit([this, pos, preload_chunk](std::stop_token st)
        {
            if (st.stop_requested())
                return;
            if (!wanted(pos, m_chunk_distance + 1))
            {
                m_cancelled_chunks.enqueue(pos);
                return;
            }
            auto chunk = std::make_shared<Chunk>(&m_dimension, pos.x, pos.z);
            realize_chunk(st, pos, chunk, preload_chunk);
        });
        m_remaining_chunks_to_load++;
    }
    return ready;
}

void GenScheduler::terrain_and_struct_chunk(std::stop_token token, ChunkPos pos, std::shared_ptr<PreLoadedChunk> chunk)
{
    if (token.stop_requested())
        return;
    m_gen->preload(pos.x, pos.z, chunk);

    if (token.stop_requested())
        return;
    m_gen->structure_pass(pos.x, pos.z, chunk, m_dimension);

    m_pregen_chunks_lockless.enqueue(chunk);
}

void GenScheduler::realize_chunk(std::stop_token token, ChunkPos pos, std::shared_ptr<Chunk> chunk, std::shared_ptr<PreLoadedChunk> pregen_chunk)
{
    std::string path = std::format("{}saves/{}/DIM{}/{}${}/blocks.dat", Filesystem::get_data_directory(), m_dimension.m_world->get_name(), m_dimension.id(), pos.x, pos.z);

    if (!Engine::get().is_save_disabled() && std::filesystem::exists(path))
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

        std::string path = std::format("{}saves/{}/DIM{}/{}${}/tags.dat", Filesystem::get_data_directory(), m_dimension.m_world->get_name(), m_dimension.id(), pos.x, pos.z);
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

        for (size_t i = 0; i < 16 * 16; i++)
            chunk->get_biomes()[i] = pregen_chunk->biomes[i];
    }
    else
    {
        memset((void *)chunk->get_blocks(), 0, sizeof(BlockState) * Chunk::block_count);

        m_gen->generate_chunk(chunk, pregen_chunk, m_dimension);

        // Save the initial version of the chunk.
        EXPECT(m_dimension.m_world->save_chunk(token, chunk, m_dimension.m_id));
    }

    chunk->count_blocks();
    m_chunks_lockless.enqueue(chunk);
}

void GenScheduler::unload_chunk(std::stop_token token, std::shared_ptr<Chunk> chunk)
{
    if (token.stop_requested())
        return;
    if (!Engine::get().is_save_disabled() && chunk->is_modified())
    {
        std::expected<void, Error> result = m_dimension.m_world->save_chunk({}, chunk, m_dimension.m_id);
        (void)result;
    }

    // TODO: only push the ChunkPos.
    m_chunks_unload_queue.enqueue(chunk);
}

void GenScheduler::queue_unload_chunk(std::shared_ptr<Chunk> chunk)
{
    Engine::get().get_thread_pool().submit([this, chunk](std::stop_token token)
                                           { unload_chunk(token, chunk); });
}

static void add_neighbour_chunk(ChunkPos pos, std::set<ChunkPos>& chunks)
{
    for (const auto& p : {
             ChunkPos(pos.x - 1, pos.z),
             ChunkPos(pos.x + 1, pos.z),
             ChunkPos(pos.x, pos.z - 1),
             ChunkPos(pos.x, pos.z + 1),
         })
        chunks.insert(p);
}

void GenScheduler::tick(std::span<const BlockPos> origins)
{
    auto active = std::make_shared<Origins>();
    for (const auto origin : origins)
        active->emplace_back(chunk_index(origin.x), chunk_index(origin.z));
    std::atomic_store(&m_active_origins, std::shared_ptr<const Origins>(active));
    if (!active->empty() && (!m_previous_origin || (active->front().x != m_previous_origin->x || active->front().z != m_previous_origin->z)))
    {
        const auto current = active->front();
        update_order(m_previous_origin ? ChunkPos(current.x-m_previous_origin->x, current.z-m_previous_origin->z) : ChunkPos());
        m_previous_origin = current;
    }
    ChunkPos cancelled;
    while (m_cancelled_terrain.try_dequeue(cancelled))
        m_pregen_loading_queue.erase(cancelled);
    while (m_cancelled_chunks.try_dequeue(cancelled))
    {
        m_chunks_loading_queue.erase(cancelled);
        --m_remaining_chunks_to_load;
        m_scheduled_origins.clear();
    }
    m_generation_batch_size = (m_gen_distance * 2 + 1) * (m_gen_distance * 2 + 1);
    if (m_seen_generation_revision != m_dimension.m_generation_revision)
    {
        m_scheduled_origins.clear();
        m_seen_generation_revision = m_dimension.m_generation_revision;
    }

    {
        // Workers enqueue results only after preload() and structure_pass().
        // Publish them before checking whether an origin is ready.
        ZoneScopedN("Pregren flush");
        std::shared_ptr<PreLoadedChunk> pregen_chunk;
        // Worker completions can arrive in a burst. Leave the rest queued for
        // later ticks instead of letting one loading frame drain everything.
        constexpr size_t max_preloads_per_tick = 128;
        for (size_t count = 0; count < max_preloads_per_tick && m_pregen_chunks_lockless.try_dequeue(pregen_chunk); ++count)
        {
            m_dimension.m_preloaded_chunks[pregen_chunk->pos] = pregen_chunk;
            m_pregen_loading_queue.erase(pregen_chunk->pos);
        }
    }

    for (BlockPos origin : origins)
    {
        const ChunkPos middle(chunk_index(origin.x), chunk_index(origin.z));
        // Each chunk becomes eligible as soon as its own structure dependencies
        // are published; distant terrain no longer blocks the entire origin.
        if (!m_scheduled_origins.contains(middle))
        {
            terrain_pass(middle);
            if (chunk_pass(middle))
                m_scheduled_origins.insert(middle);
        }
    }

    double stage_begin = Engine::get().time();

    // Remove pregen chunks if too far from the origins.
    std::vector<ChunkPos> pregen_chunks_to_remove;
    for (auto iter = m_dimension.m_preloaded_chunks.begin(); iter != m_dimension.m_preloaded_chunks.end(); ++iter)
    {
        const ChunkPos pos = iter->first;
        bool used = false;

        for (BlockPos origin : origins)
        {
            const int64_t cx = chunk_index(origin.x);
            const int64_t cz = chunk_index(origin.z);
            const ChunkPos middle(cx, cz);
            if (std::abs(pos.x - middle.x) <= (m_chunk_distance + m_gen_distance + 1) && std::abs(pos.z - middle.z) <= (m_chunk_distance + m_gen_distance + 1))
                used = true;
        }

        if (!used)
        {
            pregen_chunks_to_remove.push_back(pos);
            if (pregen_chunks_to_remove.size() == 128)
                break;
        }
    }
    for (ChunkPos pos : pregen_chunks_to_remove)
    {
        m_dimension.m_preloaded_chunks.erase(pos);
        if (Engine::get().time() - stage_begin >= 0.002)
            break;
    }

    // Unload chunks if too far from the origins
    std::vector<std::shared_ptr<Chunk>> chunks_to_remove;
    for (const auto& [pos, chunk] : m_dimension.m_chunks)
    {
        bool used = false;

        for (BlockPos origin : origins)
        {
            const int64_t cx = chunk_index(origin.x);
            const int64_t cz = chunk_index(origin.z);
            const ChunkPos middle(cx, cz);
            if (std::abs(middle.x - pos.x) <= m_chunk_distance + 1 && std::abs(middle.z - pos.z) <= m_chunk_distance + 1)
                used = true;
        }

        if (!used)
        {
            chunks_to_remove.push_back(chunk);
            if (chunks_to_remove.size() == 8)
                break;
        }
    }
    for (std::shared_ptr<Chunk> chunk : chunks_to_remove)
    {
        const ChunkPos pos = chunk->pos();
        m_dimension.m_chunks.erase(pos);
        m_dimension.m_chunks_rebuild_queue.erase(pos);
        m_dimension.m_mesh_requests.erase(pos);
        m_dimension.m_meshed_chunks.erase(pos);
        queue_unload_chunk(chunk);
    }

    if (!pregen_chunks_to_remove.empty() || !chunks_to_remove.empty())
        m_scheduled_origins.clear();

    std::set<ChunkPos> chunk_modified;

    {
        ZoneScopedN("Chunk flush");
        std::shared_ptr<Chunk> chunk;
        constexpr size_t max_chunks_per_tick = 8;
        for (size_t count = 0; count < max_chunks_per_tick && m_chunks_lockless.try_dequeue(chunk); ++count)
        {
            const ChunkPos pos = chunk->pos();
            m_chunks_loading_queue.erase(pos);
            --m_remaining_chunks_to_load;
            if (!wanted(pos, m_chunk_distance + 1))
            {
                queue_unload_chunk(chunk);
                continue;
            }
            if (m_dimension.m_chunks.contains(pos))
                continue;
            m_dimension.m_chunks[pos] = chunk;
            chunk_modified.insert(pos);
            add_neighbour_chunk(pos, chunk_modified);
        }
    }

    stage_begin = Engine::get().time();
    {
        ZoneScopedN("Chunk unload");
        std::shared_ptr<Chunk> chunk;
        // A single final release can stall inside GPU buffer destruction for
        // much longer than a frame. Transfer ownership to a worker instead of
        // destroying completed unloads here; a time budget cannot interrupt a
        // destructor already running on the main thread.
        for (size_t count = 0; count < 8 && m_chunks_unload_queue.try_dequeue(chunk); ++count)
        {
            const ChunkPos pos = chunk->pos();
            chunk_modified.insert(pos);
            add_neighbour_chunk(pos, chunk_modified);
            Engine::get().get_thread_pool().submit(
                [retired = std::move(chunk)](std::stop_token) mutable
                {
                    // Cleanup is required even when cancellation was requested.
                    retired.reset();
                });
            if (Engine::get().time() - stage_begin >= 0.002)
                break;
        }
    }

    for (ChunkPos pos : chunk_modified)
    {
        m_dimension.queue_rebuild(pos);
    }
}

float GenScheduler::get_generation_progression()
{
    const auto origins = std::atomic_load(&m_active_origins);
    if (!origins || origins->empty())
        return 0.0f;
    size_t loaded = 0, total = 0;
    for (const auto origin : *origins)
        for (int64_t x = -m_chunk_distance; x <= m_chunk_distance; ++x)
            for (int64_t z = -m_chunk_distance; z <= m_chunk_distance; ++z)
            {
                ++total;
                loaded += m_dimension.m_meshed_chunks.contains({origin.x+x, origin.z+z});
            }
    return float(loaded) / float(total);
}

Dimension::Dimension(int id)
    : m_id(id)
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
        if (entity->id() == id && entity->get_dimension() == m_id)
            return entity;
    }

    // Packets can target an entity in the same frame as AddEntity, before
    // World commits the deferred addition.
    for (const auto& entity : m_entities_to_add)
    {
        if (entity->id() == id && entity->get_dimension() == m_id)
            return entity;
    }

    return nullptr;
}

std::vector<AABBd> Dimension::get_boxes_that_may_collide(const AABBd& box) const
{
    std::vector<AABBd> boxes;
    // Blocks are centered on integer coordinates and extend by half a block.
    // Query the complete supplied bounds, including the movement sweep.
    const int64_t min_x = int64_t(std::floor(box.min.x + 0.5));
    const int64_t max_x = int64_t(std::floor(box.max.x + 0.5));
    const int64_t min_y = int64_t(std::floor(box.min.y + 0.5));
    const int64_t max_y = int64_t(std::floor(box.max.y + 0.5));
    const int64_t min_z = int64_t(std::floor(box.min.z + 0.5));
    const int64_t max_z = int64_t(std::floor(box.max.z + 0.5));

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
        if (entity->get_dimension() == m_id && entity->get_aabb().translate(entity->get_position()).intersect(box))
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
    ZoneScoped;
    if (slice_count == 0)
        return;

    const auto fail = [&] {
        MeshRebuildResult result;
        result.pos = chunk->pos();
        result.chunk = chunk;
        result.failed = true;
        m_mesh_queue_lockless.enqueue(std::move(result));
    };
    MeshRebuildResult results[16];
    MeshBuilder shadow_builder;
    // A partial rebuild cannot replace the merged mesh without dropping the
    // untouched slices. Block edits currently queue full-chunk rebuilds; any
    // future partial-update path must also refresh the complete shadow mesh.
    const bool rebuild_shadow_mesh = slice_index == 0 && slice_count == Chunk::slice_count;

    // Resolve tag maps once per rebuild, not for every voxel and its six
    // neighbors in each slice. These masks live only for this worker task.
    std::map<ChunkPos, Chunk::WaterMask> water_masks;
    water_masks.emplace(chunk->pos(), chunk->build_water_mask());
    if (water_masks.begin()->second.slices.any())
        for (const auto& [pos, neighbor] : nchunks)
            water_masks.emplace(pos, neighbor->build_water_mask());

    for (size_t i = slice_index; i < slice_index + slice_count; i++)
    {
        if (token.stop_requested())
            return;

        results[i].pos = chunk->pos();
        results[i].chunk = chunk;
        results[i].slice_index = i;

        std::expected<std::shared_ptr<Mesh>, Error> opaque_mesh = chunk->build_opaque_mesh(i, nchunks, rebuild_shadow_mesh ? &shadow_builder : nullptr);
        if (!opaque_mesh.has_value())
            return fail();
        std::expected<std::shared_ptr<Mesh>, Error> water_mesh = chunk->build_water_mesh(i, water_masks);
        if (!water_mesh.has_value())
            return fail();

        results[i].opaque_mesh = opaque_mesh.value();
        results[i].water_mesh = water_mesh.value();
    }

    if (rebuild_shadow_mesh)
    {
        // Publish once for the whole chunk. The flag also publishes a null mesh
        // when the chunk becomes empty, clearing its previous shadow geometry.
        results[slice_index].updates_shadow_mesh = true;
        if (shadow_builder.vertex_count() > 0)
        {
            std::expected<std::shared_ptr<Mesh>, Error> shadow_mesh = shadow_builder.build_positions();
            if (!shadow_mesh.has_value())
                return fail();
            results[slice_index].shadow_mesh = shadow_mesh.value();
        }
    }

    results[slice_index + slice_count - 1].completes_rebuild = true;
    m_mesh_queue_lockless.enqueue_bulk(std::begin(results) + slice_index, slice_count);
}

size_t Dimension::flush_meshes(size_t max_results)
{
    MeshRebuildResult result;
    size_t processed = 0;
    const double publish_begin = Engine::get().time();
    while (processed < max_results && m_mesh_queue_lockless.try_dequeue(result))
    {
        ++processed;
        // Do not attach an unloaded chunk's late results to its replacement.
        const auto it = m_chunks.find(result.pos);
        if (it == m_chunks.end() || it->second != result.chunk)
            continue;
        if (result.failed)
        {
            m_chunks_rebuild_queue.erase(result.pos);
            queue_rebuild(result.pos);
            continue;
        }
        const auto& chunk = it->second;
        chunk->get_slices()[result.slice_index].opaque_mesh = std::move(result.opaque_mesh);
        chunk->get_slices()[result.slice_index].water_mesh = std::move(result.water_mesh);
        if (result.updates_shadow_mesh)
            chunk->set_shadow_mesh(std::move(result.shadow_mesh));
        // A bounded flush may stop partway through a chunk. Keep it in flight
        // until its last slice arrives, so pending edits cannot race a rebuild
        // against slices still waiting to be published from the previous one.
        if (result.completes_rebuild && m_chunks_rebuild_queue.erase(result.pos) > 0)
            m_meshed_chunks.insert(result.pos);
        // Allow enough throughput for sprint flight, but stop between results
        // when publishing or retiring GPU resources consumes the frame budget.
        if (Engine::get().time() - publish_begin >= 0.002)
            break;
    }
    return processed;
}

void Dimension::queue_rebuild(ChunkPos pos, size_t slice_index, size_t slice_count)
{
    ZoneScoped;
    if (slice_count == 0)
        return;

    if (!m_chunks.contains(pos))
        return;
    auto [it, inserted] = m_mesh_requests.try_emplace(pos, slice_index, slice_count);
    if (!inserted)
    {
        const size_t end = std::max(it->second.first + it->second.second, slice_index + slice_count);
        it->second.first = std::min(it->second.first, slice_index);
        it->second.second = end - it->second.first;
    }
}

void Dimension::dispatch_meshes()
{
    // Submit only enough work to occupy workers. The remaining requests stay
    // reprioritizable instead of burying newly visible chunks in a FIFO backlog.
    const size_t limit = Engine::get().get_mesh_thread_pool().size() * 4;
    if (m_chunks_rebuild_queue.size() >= limit || m_mesh_requests.empty())
        return;
    glm::dvec3 origin(0.0);
    glm::dvec3 ahead(0.0);
    if (m_world && m_world->get_player())
    {
        origin = m_world->get_player()->get_position();
        ahead = m_world->get_player()->streaming_velocity() * ChunkStreaming::prediction_seconds;
        ahead.y = 0.0;
        const double distance = glm::length(ahead);
        if (distance > ChunkStreaming::max_prediction_blocks)
            ahead *= ChunkStreaming::max_prediction_blocks / distance;
        ahead /= double(Chunk::width);
    }
    const double cx = origin.x / Chunk::width;
    const double cz = origin.z / Chunk::width;
    std::vector<std::pair<double, ChunkPos>> candidates;
    for (auto it = m_mesh_requests.begin(); it != m_mesh_requests.end();)
    {
        const auto pos = it->first;
        if (!m_chunks.contains(pos))
        {
            it = m_mesh_requests.erase(it);
            continue;
        }
        if (!m_chunks_rebuild_queue.contains(pos))
        {
            const double x = double(pos.x) + 0.5 - cx;
            const double z = double(pos.z) + 0.5 - cz;
            // Nearby edits remain responsive; missing terrain gets precedence
            // over equally distant seam updates to already visible chunks.
            const double distance = x*x + z*z;
            // Always repair the immediate neighborhood first. Otherwise rank
            // against the predicted position, so rear seam updates cannot bury
            // terrain that sprint flight is about to reveal.
            const double priority = distance <= 4.0 ? distance - 10000.0
                : distance - 2.0 * (x * ahead.x + z * ahead.z);
            candidates.emplace_back(priority + (m_meshed_chunks.contains(pos) ? 16.0 : 0.0), pos);
        }
        ++it;
    }
    const size_t dispatch_count = std::min(candidates.size(), limit - m_chunks_rebuild_queue.size());
    std::partial_sort(candidates.begin(), candidates.begin() + dispatch_count, candidates.end());
    candidates.resize(dispatch_count);
    for (const auto& [score, pos] : candidates)
    {
        if (m_chunks_rebuild_queue.size() >= limit)
            break;
        const auto [slice_index, slice_count] = m_mesh_requests.at(pos);
        m_mesh_requests.erase(pos);
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
                continue;
            nchunks[p] = chunk_opt.value();
        }

        m_chunks_rebuild_queue.insert(pos);
        Engine::get().get_mesh_thread_pool().submit([this, chunk, nchunks, slice_index, slice_count](std::stop_token token)
                                                    { rebuild(token, chunk, nchunks, slice_index, slice_count); });
    }
}

void Dimension::remove_preload(ChunkPos pos)
{
    if (m_preloaded_chunks.erase(pos) > 0)
        ++m_generation_revision;
}

void Dimension::update_sun(glm::mat4 matrix)
{
    ZoneScoped;

    m_sun_frustum = Frustum(matrix);
}

void Dimension::place_structure(glm::i64vec3 pos, const std::vector<BlockState>& blocks, int64_t w, int64_t h, int64_t l)
{
    ZoneScoped;

    if (w <= 0 || h <= 0 || l <= 0)
        return;
    auto structure = std::make_shared<const StructureGen>(pos, blocks, w, h, l);
    std::lock_guard<std::mutex> lock(m_structures_mutex);
    for (int64_t x = chunk_index(pos.x); x <= chunk_index(pos.x+w-1); ++x)
        for (int64_t z = chunk_index(pos.z); z <= chunk_index(pos.z+l-1); ++z)
            m_structures_by_chunk[{x,z}].push_back(structure);
}

void Dimension::get_structures_overlap(ChunkPos pos, std::vector<StructureGen>& structures)
{
    ZoneScoped;

    std::vector<std::shared_ptr<const StructureGen>> candidates;
    {
        std::lock_guard<std::mutex> lock(m_structures_mutex);
        const auto it = m_structures_by_chunk.find(pos);
        if (it == m_structures_by_chunk.end())
            return;
        candidates = it->second;
    }

    const AABBi chunk_box(glm::i64vec3(pos.x * 16, 0, pos.z * 16),
                          glm::i64vec3(pos.x * 16 + 16, 256, pos.z * 16 + 16));

    for (const auto& candidate : candidates)
    {
        const StructureGen& structure = *candidate;
        const AABBi box(structure.pos,
                        structure.pos + glm::i64vec3(structure.w, structure.h, structure.l));

        if (box.intersect(chunk_box))
            structures.push_back(structure);
    }
}

size_t Dimension::count_triangles()
{
    size_t count = 0;
    for (const auto& [pos, r] : m_visible_chunks)
        for (size_t index : r.slice_indices)
        {
            if (r.chunk->get_slices()[index].opaque_mesh != nullptr)
                count += r.chunk->get_slices()[index].opaque_mesh->vertex_count() / 3;
            if (r.chunk->get_slices()[index].water_mesh != nullptr)
                count += r.chunk->get_slices()[index].water_mesh->vertex_count() / 3;
        }
    return count;
}

size_t Dimension::count_blocks()
{
    size_t count = 0;
    for (const auto& [pos, chunk] : m_chunks)
        count += chunk->get_block_count();
    return count;
}

void Dimension::write_tags(Writer& writer, std::shared_ptr<Chunk> chunk)
{
    ZoneScoped;

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
    ZoneScoped;

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
