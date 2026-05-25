#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Ref.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Profiler.hpp"
#include "World/Biome.hpp"
#include "World/Chunk.hpp"
#include "World/Pass/Flat.hpp"
#include "World/Pass/Overworld.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <mutex>

// https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
static bool ray_intersect_aabb(const Ray& ray, const AABB& aabb, float& t_min, glm::vec3& normal)
{
    glm::vec3 inv_dir = 1.0f / ray.dir();
    glm::vec3 t0s = (aabb.min - ray.origin()) * inv_dir;
    glm::vec3 t1s = (aabb.max - ray.origin()) * inv_dir;

    glm::vec3 tsmaller = glm::min(t0s, t1s);
    glm::vec3 tbigger = glm::max(t0s, t1s);

    t_min = std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z));
    float t_max = std::min(tbigger.x, std::min(tbigger.y, tbigger.z));

    if (t_min > t_max)
        return false;

    const float eps = 1e-6f;
    if (t_min == tsmaller.x || std::abs(t_min - tsmaller.x) < eps)
        normal = (inv_dir.x >= 0.0f) ? glm::vec3(-1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    else if (t_min == tsmaller.y || std::abs(t_min - tsmaller.y) < eps)
        normal = (inv_dir.y >= 0.0f) ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    else
        normal = (inv_dir.z >= 0.0f) ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 0.0f, 1.0f);

    return true;
}

World::World()
{
}

void World::find_safe_spawn()
{
    srand(0);
    glm::vec3 water_spawn_position;
    uint16_t water_id = Engine::get().blocks().get_block_id("water");
    bool found = false;
    size_t i;

    for (i = 0; i < 30 && !found; i++)
    {
        int64_t x = rand() % 30;
        int64_t z = rand() % 30;

        for (int64_t y = Chunk::height - 1; y > 3; y -= 4)
        {
            BlockState state = m_dims[overworld].generate_block(x, y, z);
            BlockState state1 = m_dims[overworld].generate_block(x, y - 1, z);
            BlockState state2 = m_dims[overworld].generate_block(x, y - 2, z);
            BlockState state3 = m_dims[overworld].generate_block(x, y - 3, z);

            if (state.is_air() && state1.is_air() && state2.is_air() && !state3.is_air())
            {
                if (state3.id != water_id)
                {
                    m_spawn_position = glm::vec3(x, y - 3, z) + glm::vec3(0, 6.6, 0);
                    found = true;
                    break;
                }
                else
                {
                    water_spawn_position = glm::vec3(x, y - 3, z) + glm::vec3(0, 6.6, 0);
                }
            }
        }
    }

    if (!found)
        m_spawn_position = water_spawn_position;
}

Result<Ref<World>> World::create(String name, uint64_t seed, int type)
{
    Ref<World> world = TRY(newref<World>());
    world->m_seed = seed;
    world->m_name = name;

    if (type == WorldPresetFlat)
    {
        EXPECT(world->m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatSurfacePass>())));
    }
    else if (type == WorldPresetNormal)
    {
        EXPECT(world->m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldSurfacePass>())));
    }

    world->find_safe_spawn();

    String path = format("{}saves/{}/", Filesystem::get_data_directory(), name);
    TRY(Filesystem::make_dirs(path));
    path.append("info.dat");
    File file = TRY(Filesystem::open_file(path, true));

    WorldSaveInfo wi{};
    wi.seed = seed;
    wi.type = WorldPresetType(type);
    wi.spawn_position = world->get_spawn_position();
    TRY(file.writer().write_raw(&wi, sizeof(WorldSaveInfo)));
    file.close();

    return world;
}

Result<Ref<World>> World::create_proxy(uint64_t seed)
{
    Ref<World> world = EXPECT(newref<World>());
    world->m_seed = seed;
    world->m_proxy = true;
    return world;
}

Result<Ref<World>> World::load(StringView name)
{
    Ref<World> world = TRY(newref<World>());

    String path = format("{}saves/{}/info.dat", Filesystem::get_data_directory(), name);
    File file = TRY(Filesystem::open_file(path));

    WorldSaveInfo wi{};
    TRY(file.reader().read_raw(&wi, sizeof(WorldSaveInfo)));
    file.close();

    world->m_name = name;
    world->m_seed = wi.seed;
    world->m_spawn_position = wi.spawn_position;

    if (wi.type == WorldPresetFlat)
    {
        EXPECT(world->m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatSurfacePass>())));
    }
    else if (wi.type == WorldPresetNormal)
    {
        EXPECT(world->m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldSurfacePass>())));
    }

    return world;
}

World::~World()
{
}

void World::tick(float delta)
{
    ZoneScoped;

    for (Ref<Entity> entity : m_dims[overworld].get_entities())
    {
        if (entity->is_active())
            entity->recurse_tick(delta);
        else
            remove_entity(entity->m_dimension, entity);
    }

    for (Ref<Entity> entity : m_dims[0].m_entities_to_remove)
    {
        m_dims[0].m_entities.remove(entity);
    }
    m_dims[0].m_entities_to_remove.clear();
    for (Ref<Entity> entity : m_dims[0].m_entities_to_add)
    {
        EXPECT(m_dims[0].m_entities.append(entity));
        // entity->recurse_tick(delta);
    }
    m_dims[0].m_entities_to_add.clear();

    if (!m_proxy)
    {
        // TODO: put this in a queue.
        for (auto [pos, chunk] : m_dims[0].m_chunks)
        {
            if (chunk->is_modified())
                EXPECT(save_chunk(chunk));
            chunk->clear_modified();
        }

        for (const Ref<Entity>& entity : m_dims[World::overworld].get_entities())
        {
            if (Ref<Player> player = entity.cast_to<Player>())
                EXPECT(save_player(player));
        }

        load_around_player();

        // Flush all new chunks.
        std::unique_lock<std::mutex> lock(m_dims[0].mutex());
        for (auto [pos, chunk] : m_dims[0].m_chunks_to_flush)
        {
            EXPECT(m_dims[0].add_chunk(pos.x, pos.z, chunk));

            /*TODO
            if (Engine::singleton->is_server())
            {
                ChunkDataPacket p;
                p.x = pos.x;
                p.z = pos.z;

                EXPECT(p.blocks.resize(Chunk::block_count_with_overlap));
                std::memcpy(p.blocks.data(), chunk->get_blocks(), sizeof(BlockState) * p.blocks.size());

                EXPECT(p.biomes.resize(Chunk::block_count_with_overlap));
                std::memcpy(p.biomes.data(), chunk->get_blocks(), sizeof(Biome) * p.biomes.size());

                Engine::singleton->get_connection().broadcast(Engine::singleton->get_connection().create_packet(p));
            }*/
        }
        m_dims[0].m_chunks_to_flush.clear();
    }
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    return m_dims[overworld].get_block(x, y, z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    m_dims[overworld].set_block(x, y, z, state);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[overworld].get_chunk(x, z);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z)
{
    return m_dims[overworld].get_chunk(x, z);
}

void World::set_active_camera(Ref<Camera> camera)
{
    m_camera = camera;
}

void World::load_around_player()
{
    const glm::vec3 player_pos = m_camera->get_global_transform().position();
    int64_t player_cx = int64_t(player_pos.x / 16);
    int64_t player_cz = int64_t(player_pos.z / 16);

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++)
        for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++)
        {
            int64_t x = player_cx + cx;
            int64_t z = player_cz + cz;
            ChunkPos pos(x, z);

            {
                std::lock_guard<std::mutex> lock(m_dims[0].mutex());
                if (m_dims[0].has_chunk(x, z) || m_dims[0].m_chunks_to_flush.contains(pos))
                    continue;
            }

            {
                std::lock_guard<std::mutex> lock(m_dims[0].m_chunk_loading_mutex);
                if (m_dims[0].m_chunk_loading_queue.contains(pos))
                    continue;

                m_dims[0].m_chunk_loading_queue.insert(pos);
            }

            EXPECT(m_generation_thread_pool.async([this, pos]
                                                  { load_one_chunk(pos); }));
        }

    std::unique_lock<std::mutex> lock(m_dims[0].mutex());
    for (const auto& [pos, chunk] : m_dims[0].m_chunks)
    {
        if (pos.x >= player_cx - m_load_distance && pos.x <= player_cx + m_load_distance && pos.z >= player_cz - m_load_distance && pos.z <= player_cz + m_load_distance)
        {
            continue;
        }

        EXPECT(m_generation_thread_pool.async([this, pos]
                                              { unload_one_chunk(pos); }));
    }
}

inline void adjust_on_boundary(double rcomp, int64_t& vcomp, double dcomp, double eps = 1e-12)
{
    if (std::abs(rcomp - static_cast<double>(vcomp)) <= eps && dcomp < 0.0)
    {
        --vcomp;
    }
}

bool World::raycast(const Ray& ray, float range, RaycastResult& result)
{
    bool hit = false;
    bool is_entiy = false;
    float t_min = std::numeric_limits<float>::infinity();
    glm::i64vec3 block_pos;
    Ref<Entity> entity;
    glm::vec3 normal;

    for (const Ref<Entity>& e : m_dims[overworld].get_entities())
    {
        float t = 0.0f;
        if (ray_intersect_aabb(ray, e->m_aabb, t, normal) && t < t_min)
        {
            t_min = t;
            hit = true;
            is_entiy = true;
            entity = e;
        }
    }

    float d = 0.0f;
    while (d <= range)
    {
        glm::vec3 pos = ray.at(d);
        glm::i64vec3 ipos(glm::round(pos));
        float t;
        if (!get_block_state(ipos.x, ipos.y, ipos.z).is_air() && ray_intersect_aabb(ray, AABB(-glm::vec3(0.5), glm::vec3(0.5)).translate(pos), t, normal) && t < t_min)
        {
            t_min = t;
            hit = true;
            is_entiy = false;
            block_pos = ipos;
        }
        d += range / 10.0f;
    }

    if (hit)
    {
        result.hit_entity = is_entiy;
        result.pos = ray.at(t_min);
        result.distance = t_min;
        result.block_pos = block_pos;
        result.normal = normal;
        return true;
    }
    return false;
}

void World::break_block(int64_t x, int64_t y, int64_t z)
{
    BlockState state = get_block_state(x, y, z);
    set_block_state(x, y, z, BlockState());

    Ref<Block> block = Engine::get().blocks().get_block_by_id(state.id);
    Ref<ItemEntity> item_entity = EXPECT(newref<ItemEntity>(block));
    item_entity->set_position(glm::vec3(x, y, z) + glm::vec3(rand_float(-0.6, 0.6), 0, rand_float(-0.6, 0.6)));

    add_entity(World::overworld, item_entity);
}

Result<void> World::save_chunk(const Ref<Chunk>& chunk)
{
    CompressedChunk cchunk = TRY(chunk->compress());

    String path = format("{}/saves/{}/DIM0/{}${}/", Filesystem::get_data_directory(), m_name, chunk->x(), chunk->z());
    TRY(Filesystem::make_dirs(path));

    path.append("blocks.dat");
    File file = TRY(Filesystem::open_file(path, true));

    WorldBlocks wb{};
    wb.padding0 = 0;
    wb.chunk_slice_mask = cchunk.compressed_slice_mask;
    wb.blocks_offset = sizeof(WorldBlocks);
    // wb.blocks_len = cchunk.compressed_blocks.size();
    // wb.blocks_tree_offset = wb.blocks_offset + cchunk.compressed_blocks.size() * sizeof(BlockState);
    // wb.blocks_tree_len = cchunk.compressed_nodes.size();
    // wb.biomes_offset = wb.blocks_tree_offset + cchunk.compressed_nodes.size() * sizeof(ChunkNode);
    // wb.biomes_len = cchunk.compressed_biomes.size();
    // wb.biomes_tree_offset = wb.biomes_offset + cchunk.compressed_biomes.size() * sizeof(Biome);
    // wb.biomes_tree_len = cchunk.compressed_biome_nodes.size();
    wb.biomes_offset = Chunk::block_count * sizeof(BlockState);

    TRY(file.writer().write_raw(&wb, sizeof(WorldBlocks)));
    TRY(file.writer().write_raw(chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count));
    TRY(file.writer().write_raw(chunk->get_biomes(), sizeof(Biome) * Chunk::block_count));
    // TRY(file.write_raw(cchunk.compressed_blocks.data(), cchunk.compressed_blocks.size() * sizeof(BlockState)));
    // TRY(file.write_raw(cchunk.compressed_nodes.data(), cchunk.compressed_nodes.size() * sizeof(ChunkNode)));
    // TRY(file.write_raw(cchunk.compressed_biomes.data(), cchunk.compressed_biomes.size() * sizeof(Biome)));
    // TRY(file.write_raw(cchunk.compressed_biome_nodes.data(), cchunk.compressed_biome_nodes.size() * sizeof(ChunkBiomeNode)));

    file.close();

    return Result<void>();
}

Result<void> World::save_entity(const Ref<Entity>& entity)
{
    int64_t cx = chunk_index(int64_t(entity->get_position().x));
    int64_t cz = chunk_index(int64_t(entity->get_position().z));

    String path = format("{}saves/{}/DIM0/{}${}/entities/", Filesystem::get_data_directory(), m_name, cx, cz);
    TRY(Filesystem::make_dirs(path));

    EntitySerializer serializer;
    TRY(serializer.set("position", entity->get_position()));
    TRY(serializer.set("rotation", entity->get_rotation()));
    entity->save(serializer);

    path.append("");

    return Result<void>();
}

Result<void> World::save_player(const Ref<Player>& player)
{
    String path = format("{}saves/{}/players/", Filesystem::get_data_directory(), m_name);
    TRY(Filesystem::make_dirs(path));

    path.append("player.dat");

    EntitySerializer serializer;
    TRY(serializer.set("position", player->get_position()));
    TRY(serializer.set("rotation", player->get_rotation()));
    player->save(serializer);

    TRY(serializer.save(path));

    return Result<void>();
}

void World::force_load_chunk_for(glm::vec3 position)
{
    int64_t chunk_x = chunk_index(int64_t(position.x));
    int64_t chunk_z = chunk_index(int64_t(position.z));
    load_one_chunk(ChunkPos(chunk_x, chunk_z));
}

bool World::is_player_saved(const StringView& name) const
{
    String path = format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), m_name, name);
    return Filesystem::exists(path);
}

void World::load_one_chunk(ChunkPos pos)
{
    Ref<Chunk> chunk;

    String path = format("{}saves/{}/DIM0/{}${}/blocks.dat", Filesystem::get_data_directory(), m_name, pos.x, pos.z);
    if (Filesystem::exists(path))
    {
        chunk = EXPECT(newref<Chunk>(pos.x, pos.z));

        LocalVector<char> data;
        // TODO: how to handle errors from loading chunks ?
        File file = EXPECT(Filesystem::open_file(path));
        EXPECT(file.reader().read_to_buffer(data));
        file.close();

        WorldBlocks wb = *(WorldBlocks *)data.data();
        // println("size = {}", data.size());
        // println("{} - {}", wb.blocks_offset, wb.blocks_offset + wb.blocks_len * sizeof(BlockState));
        // println("{} - {}", wb.blocks_tree_offset, wb.blocks_tree_offset + wb.blocks_tree_len * sizeof(ChunkNode));
        // println("{} - {}", wb.biomes_offset, wb.biomes_offset + wb.biomes_len * sizeof(Biome));
        // println("{} - {}\n", wb.biomes_tree_offset, wb.biomes_tree_offset + wb.biomes_tree_len * sizeof(ChunkBiomeNode));

        // some sanity checks
        // if (
        //     wb.blocks_offset + wb.blocks_len * sizeof(BlockState) > data.size() ||
        //     wb.blocks_tree_offset + wb.blocks_tree_len * sizeof(ChunkNode) > data.size() ||
        //     wb.biomes_offset + wb.biomes_len * sizeof(Biome) > data.size() ||
        //     wb.biomes_tree_offset + wb.biomes_tree_len * sizeof(ChunkBiomeNode) > data.size())
        // {
        //     return;
        // }

        memcpy(chunk->get_blocks(), data.data() + wb.blocks_offset, sizeof(BlockState) * Chunk::block_count);
        memcpy(chunk->get_biomes(), data.data() + wb.biomes_offset, sizeof(BlockState) * Chunk::block_count);

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            chunk->get_slices()[i].empty = false;
            EXPECT(chunk->build_simple_mesh(i));
        }

        // memset(chunk->get_blocks(), 0, sizeof(BlockState) * Chunk::block_count);
        // memset(chunk->get_biomes(), 0, sizeof(Biome) * Chunk::block_count);

        // EXPECT(chunk->uncompress(
        //     View((BlockState *)(data.data() + wb.blocks_offset), wb.blocks_len),
        //     View((ChunkNode *)(data.data() + wb.blocks_tree_offset), wb.blocks_tree_len),
        //     wb.chunk_slice_mask,
        //     View((Biome *)(data.data() + wb.biomes_offset), wb.biomes_len),
        //     View((ChunkBiomeNode *)(data.data() + wb.biomes_tree_offset), wb.biomes_tree_len)));
    }
    else
    {
        Result<Ref<Chunk>> result = m_dims[0].generate_chunk(pos.x, pos.z);
        if (result.has_error())
            return;
        chunk = result.value();

        EXPECT(save_chunk(chunk));
    }

    {
        std::lock_guard<std::mutex> lock(m_dims[0].mutex());
        m_dims[0].m_chunks_to_flush[pos] = chunk;
    }
    {
        std::lock_guard<std::mutex> lock(m_dims[0].m_chunk_loading_mutex);
        m_dims[0].m_chunk_loading_queue.erase(pos);
    }
}

void World::unload_one_chunk(ChunkPos pos)
{
    std::unique_lock<std::mutex> lock(m_dims[0].mutex());
    m_dims[0].remove_chunk(pos.x, pos.z);
}
