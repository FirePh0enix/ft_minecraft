#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Print.hpp"
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

#include <zlib.h>

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

    float t_near = std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z));
    float t_far = std::min(tbigger.x, std::min(tbigger.y, tbigger.z));

    if (t_near > t_far)
        return false;

    if (t_far < 0.0f)
        return false;

    t_min = (t_near >= 0.0f) ? t_near : t_far;

    const float eps = 1e-6f;
    normal = glm::vec3(0.0f);

    if (std::abs(t_min - t0s.x) < eps || std::abs(t_min - t1s.x) < eps)
    {
        normal.x = (t_min == t0s.x) ? -glm::sign(inv_dir.x) : glm::sign(inv_dir.x);
        return true;
    }
    if (std::abs(t_min - t0s.y) < eps || std::abs(t_min - t1s.y) < eps)
    {
        normal.y = (t_min == t0s.y) ? glm::sign(inv_dir.y) : -glm::sign(inv_dir.y);
        return true;
    }
    if (std::abs(t_min - t0s.z) < eps || std::abs(t_min - t1s.z) < eps)
    {
        normal.z = (t_min == t0s.z) ? -glm::sign(inv_dir.z) : glm::sign(inv_dir.z);
        return true;
    }

    if (t_near == t_min)
    {
        if (t_near == tsmaller.x)
            normal = glm::vec3(-glm::sign(inv_dir.x), 0.0f, 0.0f);
        else if (t_near == tsmaller.y)
            normal = glm::vec3(0.0f, -glm::sign(inv_dir.y), 0.0f);
        else
            normal = glm::vec3(0.0f, 0.0f, -glm::sign(inv_dir.z));
    }
    else
    {
        if (t_min == tbigger.x)
            normal = glm::vec3(glm::sign(inv_dir.x), 0.0f, 0.0f);
        else if (t_min == tbigger.y)
            normal = glm::vec3(0.0f, -glm::sign(inv_dir.y), 0.0f);
        else
            normal = glm::vec3(0.0f, 0.0f, glm::sign(inv_dir.z));
    }

    return true;
}

World::World()
{
}

void World::find_safe_spawn()
{
    srand(0);
    glm::vec3 water_spawn_position;
    bool found = false;
    size_t i;

    for (i = 0; i < 30 && !found; i++)
    {
        int64_t x = rand() % 30;
        int64_t z = rand() % 30;

        for (int64_t y = Chunk::height - 1; y > 3; y -= 4)
        {
            // BlockState state = m_dims[overworld].generate_block(x, y, z);
            // BlockState state1 = m_dims[overworld].generate_block(x, y - 1, z);
            // BlockState state2 = m_dims[overworld].generate_block(x, y - 2, z);
            // BlockState state3 = m_dims[overworld].generate_block(x, y - 3, z);

            // if (state.is_air() && state1.is_air() && state2.is_air() && !state3.is_air())
            // {
            // if (state3.id != Blocks::water)
            // {
            m_spawn_position = glm::vec3(x, y - 3, z) + glm::vec3(0, 6.6, 0);
            found = true;
            break;
            // }
            // else
            // {
            //     water_spawn_position = glm::vec3(x, y - 3, z) + glm::vec3(0, 6.6, 0);
            // }
            // }
        }
    }

    if (!found)
        m_spawn_position = water_spawn_position;
}

Result<Ref<World>> World::create(String name, uint64_t seed, int type)
{
    Ref<World> world = newref<World>();
    world->m_seed = seed;
    world->m_name = name;

    if (type == WorldPresetFlat)
    {
        world->m_dims[overworld].m_generation_passes.append(newref<FlatSurfacePass>());
    }
    else if (type == WorldPresetNormal)
    {
        world->m_dims[overworld].m_generation_passes.append(newref<OverworldSurfacePass>());
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
    Ref<World> world = newref<World>();
    world->m_seed = seed;
    world->m_proxy = true;
    return world;
}

Result<Ref<World>> World::load(StringView name)
{
    Ref<World> world = newref<World>();

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
        world->m_dims[overworld].m_generation_passes.append(newref<FlatSurfacePass>());
    }
    else if (wi.type == WorldPresetNormal)
    {
        world->m_dims[overworld].m_generation_passes.append(newref<OverworldSurfacePass>());
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
    for (Ref<Entity> entity : m_dims[0].m_entities_to_add)
    {
        m_dims[0].m_entities.append(entity);
    }

    if (!m_proxy)
    {
        for (auto& [pos, chunk] : m_dims[0].m_chunks)
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
    }

    // Flush all new chunks.
    LocalVector<ChunkPos> chunk_modified;

    {
        std::lock_guard<std::mutex> lock(m_dims[0].mutex());
        for (auto& [pos, chunk] : m_dims[0].m_chunks_to_flush)
        {
            m_dims[0].m_chunks.put(pos, chunk);
            chunk_modified.append(pos);
        }
        m_dims[0].m_chunks_to_flush.clear();

        for (auto pos : m_dims[0].m_chunks_to_remove)
        {
            m_dims[0].m_chunks.erase(pos);
            chunk_modified.append(pos);
        }
        m_dims[0].m_chunks_to_remove.clear();
    }

    // for (ChunkPos pos : chunk_modified)
    // {
    //     EXPECT(m_generation_thread_pool.async([this, pos]
    //                                           { EXPECT(m_dims[0].rebuild_neighbor_chunks_water(pos.x, pos.z)); }));
    // }

    if (!m_proxy && Engine::get().is_server()) {
	for (Ref<Entity> entity : m_dims[World::overworld].get_entities()) {
	    UpdateEntityPacket p{};
	    p.id = entity->id();
	    p.position = entity->get_transform().position();
	    p.rotation = entity->get_transform().rotation();
	    Engine::get().connection().broadcast(Engine::get().connection().create_packet(p));
	}
    }

    m_dims[0].m_entities_to_remove.clear();
    m_dims[0].m_entities_to_add.clear();
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    return m_dims[overworld].get_block(x, y, z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    m_dims[overworld].set_block(x, y, z, state);
}

Option<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[overworld].get_chunk(x, z);
}

Option<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z)
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

                m_dims[0].m_chunk_loading_queue.put(pos);
            }

            m_generation_thread_pool.async([this, pos]
                                           { load_one_chunk(pos); });
        }

    std::lock_guard<std::mutex> lock(m_dims[0].mutex());
    for (const auto& [pos, chunk] : m_dims[0].m_chunks)
    {
        if (pos.x >= player_cx - m_load_distance && pos.x <= player_cx + m_load_distance && pos.z >= player_cz - m_load_distance && pos.z <= player_cz + m_load_distance)
        {
            continue;
        }

        m_generation_thread_pool.async([this, pos]
                                       { unload_one_chunk(pos); });
    }
}

inline void adjust_on_boundary(double rcomp, int64_t& vcomp, double dcomp, double eps = 1e-12)
{
    if (std::abs(rcomp - static_cast<double>(vcomp)) <= eps && dcomp < 0.0)
    {
        --vcomp;
    }
}

bool World::raycast(const Ray& ray, float range, RaycastResult& result, const Entity *ignore)
{
    bool hit = false;
    bool is_entiy = false;
    float t_min = std::numeric_limits<float>::infinity();
    glm::i64vec3 block_pos;
    Ref<Entity> entity;
    glm::vec3 normal;

    for (const Ref<Entity>& e : m_dims[overworld].get_entities())
    {
        if (e.ptr() == ignore)
            continue;

        AABB world_aabb = e->get_aabb().translate(e->get_global_transform().position());
        float t = 0.0f;
        glm::vec3 normal;
        if (ray_intersect_aabb(ray, world_aabb, t, normal) &&
            t >= 0.0f &&
            t <= range &&
            t < t_min)
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
        result.entity = entity;
        return true;
    }
    return false;
}

void World::break_block(int64_t x, int64_t y, int64_t z)
{
    BlockState state = get_block_state(x, y, z);
    set_block_state(x, y, z, BlockState());

    Option<Id<Item>> item_opt = Engine::get().registry().to_item(Id<Block>(state.id));
    if (!item_opt.has_value())
        return;

    Ref<ItemEntity> item_entity = newref<ItemEntity>(item_opt.value());
    item_entity->set_position(glm::vec3(x, y, z) + glm::vec3(rand_float(-0.6, 0.6), 0, rand_float(-0.6, 0.6)));

    add_entity(World::overworld, item_entity);
}

Result<void> World::save_chunk(Ref<Chunk> chunk)
{
    // CompressedChunk cchunk = TRY(chunk->compress());

    String path = format("{}/saves/{}/DIM0/{}${}/", Filesystem::get_data_directory(), m_name, chunk->x(), chunk->z());
    TRY(Filesystem::make_dirs(path));

    path.append("blocks.dat");
    File file = TRY(Filesystem::open_file(path, true));

    WorldBlocks wb{};
    wb.padding0 = 0;
    // wb.chunk_slice_mask = cchunk.compressed_slice_mask;
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

    path = format("{}/saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, chunk->x(), chunk->z());
    file = TRY(Filesystem::open_file(path, true));

    Map<int64_t, Map<String, Variant>> tags;
    for (const auto& [key, value] : chunk->m_tags)
    {
        Map<String, Variant> tags2;
        for (const auto& [_, key2, value2] : value.tags)
            tags2.put(key2, value2);
        tags.put(key, tags2);
    }

    TRY(file.writer().write_variant(Variant(tags)));

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
    serializer.set("position", entity->get_position());
    serializer.set("rotation", entity->get_rotation());
    entity->save(serializer);

    path.append("");
    // TODO

    return Result<void>();
}

Result<void> World::save_player(const Ref<Player>& player)
{
    String path = format("{}saves/{}/players/", Filesystem::get_data_directory(), m_name);
    TRY(Filesystem::make_dirs(path));

    path.append("player.dat");

    EntitySerializer serializer;
    serializer.set("position", player->get_position());
    serializer.set("rotation", player->get_rotation());
    player->save(serializer);

    TRY(serializer.save(path));

    return Result<void>();
}

#define DEFLATE_BUFFER_SIZE 4 * 1024

void World::send_chunk(ENetPeer *peer, const Ref<Chunk>& chunk) const
{
    std::vector<uint8_t> buffer; // TODO: use Vector
    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = (uint8_t *)chunk->get_blocks();
    strm.avail_in = Chunk::block_count * sizeof(BlockState);
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    deflateInit(&strm, Z_BEST_COMPRESSION);

    while(strm.avail_in != 0) {
	int res = deflate(&strm, Z_NO_FLUSH);
	assert(res == Z_OK);

	if (strm.avail_out == 0) {
	    buffer.insert(buffer.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
	    strm.next_out = tmp;
	    strm.avail_out = DEFLATE_BUFFER_SIZE;
	}
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK) {
	if (strm.avail_out == 0) {
	    buffer.insert(buffer.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
	    strm.next_out = tmp;
	    strm.avail_out = DEFLATE_BUFFER_SIZE;
	}
	deflate_res = deflate(&strm, Z_FINISH);
    }

    assert(deflate_res == Z_STREAM_END);
    buffer.insert(buffer.end(), tmp, tmp + DEFLATE_BUFFER_SIZE - strm.avail_out);
    deflateEnd(&strm);

    ChunkDataPacket p;
    p.x = chunk->x();
    p.z = chunk->z();

    p.blocks.resize(buffer.size());
    memcpy(p.blocks.data(), buffer.data(), buffer.size());

    Engine::get().connection().send(peer, Engine::get().connection().create_packet(p));
}
 
#define INFLATE_BUFFER_SIZE (sizeof(BlockState) * 1024)

void World::receive_chunk(int64_t x, int64_t z, uint8_t *compressed_data, size_t size)
{
    Dimension& dimension = get_dimension(World::overworld);
    bool has_chunk = dimension.has_chunk(x, z);
    
    Ref<Chunk> chunk;
    if (has_chunk) {
	chunk = dimension.get_chunk(x, z).value();
	// TODO: maybe we need a mutex to modify a chunk, for now it only creates new one.
    } else {
	chunk = newref<Chunk>(&dimension, x, z);
    }

    z_stream strm{};
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = (uint8_t *)compressed_data;
    strm.avail_in = size;
    strm.next_out = (uint8_t *)chunk->get_blocks();
    strm.avail_out = Chunk::block_count * sizeof(BlockState);

    // TODO: Obsiously we don't want to crash on errors.
    
    // The expected size of the output is already known which allows to have only one `inflate` call.
    int res = inflateInit(&strm);
    assert(res == Z_OK);
    res = inflate(&strm, Z_FINISH);
    assert(res == Z_STREAM_END);
    inflateEnd(&strm);
    
    for (size_t i = 0; i < Chunk::slice_count; i++) {
	// TODO: Remove the `empty` boolean, this serve no actual purpose.
     	chunk->get_slices()[i].empty = false;
     	EXPECT(chunk->build_simple_mesh(i));
    }

    destroy_array_nodestruct(compressed_data, size);

    if (!has_chunk) {
    	dimension.add_chunk(x, z, chunk);
    }
}

void World::deferred_receive_chunk(int64_t x, int64_t z, uint8_t *compressed_data, size_t size)
{
    // Maybe I'm dumb and I don't know anything but using `[&]` creates segfaults, but manually specifying captures don't.
    m_generation_thread_pool.async([this, x, z, compressed_data, size]() { receive_chunk(x, z, compressed_data, size); });
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
        chunk = newref<Chunk>(&m_dims[0], pos.x, pos.z);

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
        memcpy(chunk->get_biomes(), data.data() + wb.biomes_offset, sizeof(Biome) * Chunk::block_count);

        String path = format("{}saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, pos.x, pos.z);
        if (Filesystem::exists(path))
        {
            File file = EXPECT(Filesystem::open_file(path));
            FileReader reader = file.reader();

            Option<Variant> variant = EXPECT(reader.read_variant());
            if (variant.has_value())
            {
                Map<int64_t, Map<String, Variant>> tags = variant.value().to_map<int64_t, Map<String, Variant>>();

                for (const auto& [key, value] : tags)
                {
                    BlockTags btags;
                    for (const auto& [key2, value2] : value)
                        btags.tags.put(key2, value2);
                    chunk->m_tags.put(key, btags);
                }

                file.close();
            }
        }

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            chunk->get_slices()[i].empty = false;
            EXPECT(chunk->build_simple_mesh(i));
            EXPECT(chunk->build_water_mesh(i));
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

    m_dims[0].add_chunk(pos.x, pos.z, chunk);

    {
        std::lock_guard<std::mutex> lock(m_dims[0].m_chunk_loading_mutex);
        m_dims[0].m_chunk_loading_queue.erase(pos);
    }
}

void World::unload_one_chunk(ChunkPos pos)
{
    m_dims[0].remove_chunk(pos.x, pos.z);
}
