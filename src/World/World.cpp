#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Containers/HashSet.hpp"
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
    // srand(0);
    
    // size_t i;
    // for (i = 0; i < 30; i++)
    // {
    //     int64_t x = rand() % 30;
    //     int64_t z = rand() % 30;

    //     for (int64_t y = Chunk::height - 1; y > 0; y--)
    //     {
    // 	    int64_t cx = local_coords(x);
    // 	    int64_t cz = local_coords(z);
    // 	    Ref<Chunk> chunk = newref<Chunk>(&m_dims[0], chunk_index(x), chunk_index(z));
    // 	    BlockState state = m_dims[0].generate_block(x, y, z, chunk);
	    
    // 	    if (!state.is_air() || chunk->get_tag(glm::i64vec3(cx, y, cz), "water").has_value()) {
    // 		m_spawn_position = glm::vec3(x, y, z) + glm::vec3(0, 2.6, 0);
    // 		return;
    // 	    }
    //     }
    // }
}

Result<Ref<World>> World::create(String name, uint64_t seed, int type)
{
    Ref<World> world = newref<World>();
    world->m_seed = seed;
    world->m_name = name;

    world->m_dims[overworld].m_gen_desc.add_pass(newref<OverworldOceanPass>());
    world->m_dims[overworld].m_gen_desc.add_pass(newref<MountainPass>());
    world->m_dims[overworld].m_gen_desc.add_pass(newref<OverworldTerrainPass>());
    
    // world->find_safe_spawn();

    if (!Engine::get().is_save_disabled()) {
	String path = format("{}saves/{}/", Filesystem::get_data_directory(), name);
	TRY(Filesystem::make_dirs(path));
	path.append("info.dat");
	File file = TRY(Filesystem::open_file(path, true));

	WorldSaveInfo wi{};
	wi.seed = seed;
	wi.type = WorldPresetType(type);
	wi.spawn_position = glm::vec3(0, 80, 0); // world->get_spawn_position();
	TRY(file.writer().write_raw(&wi, sizeof(WorldSaveInfo)));
	file.close();
    }

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

    if (!Engine::get().is_save_disabled()) {
	String path = format("{}saves/{}/info.dat", Filesystem::get_data_directory(), name);
	File file = TRY(Filesystem::open_file(path));

	WorldSaveInfo wi{};
	TRY(file.reader().read_raw(&wi, sizeof(WorldSaveInfo)));
	file.close();

	world->m_seed = wi.seed;
	world->m_spawn_position = wi.spawn_position;
    }

    world->m_name = name;
    
    world->m_dims[overworld].m_gen_desc.add_pass(newref<OverworldOceanPass>());
    world->m_dims[overworld].m_gen_desc.add_pass(newref<MountainPass>());
    world->m_dims[overworld].m_gen_desc.add_pass(newref<OverworldTerrainPass>());

    return world;
}

World::~World()
{
}

static void add_neighbour_chunk(ChunkPos pos, Set<ChunkPos>& chunks) {
    const Array<ChunkPos, 4> positions = {
	ChunkPos(pos.x - 1, pos.z),
	ChunkPos(pos.x + 1, pos.z),
	ChunkPos(pos.x, pos.z - 1),
	ChunkPos(pos.x, pos.z + 1),
    };

    for (const auto& p : positions) {
	chunks.put(p);
    }
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

	// TODO: Don't save every players each frames.
        for (const Ref<Entity>& entity : m_dims[World::overworld].get_entities())
        {
            if (Ref<Player> player = entity.cast_to<Player>())
                EXPECT(save_player(player));
        }

        load_around_player();
    } else if (Engine::get().is_online() && !Engine::get().is_server()) {
	request_load_around();
    }

    // Flush all new chunks.
    Set<ChunkPos> chunk_modified;
    
    {	
        std::lock_guard<std::mutex> lock(m_dims[0].mutex());
	
        for (auto& [pos, chunk] : m_dims[0].m_chunks_to_flush) {
            m_dims[0].m_chunks.put(pos, chunk);
            chunk_modified.put(pos);
	    add_neighbour_chunk(pos, chunk_modified);
        }
        m_dims[0].m_chunks_to_flush.clear();

        for (auto pos : m_dims[0].m_chunks_to_remove) {
            m_dims[0].m_chunks.erase(pos);
            chunk_modified.put(pos);
	    add_neighbour_chunk(pos, chunk_modified);
        }
        m_dims[0].m_chunks_to_remove.clear();
    }

    for (ChunkPos pos : chunk_modified) {
        m_dims[World::overworld].queue_rebuild(pos);
    }
 
    if (!m_proxy && Engine::get().is_online() && Engine::get().is_server()) {
	for (Ref<Entity> entity : m_dims[World::overworld].get_entities()) {
	    UpdateEntityPacket p{};
	    p.id = entity->id();
	    p.position = entity->get_transform().position();
	    p.rotation = entity->get_transform().rotation();
	    Engine::get().connection().broadcast(Engine::get().connection().create_packet(p));
	}

	for (const ChunkLoadRequest& req : m_load_requests) {
	    Option<Ref<Chunk>> chunk_opt = get_dimension(req.dimension).get_chunk(req.x, req.z);
	    if (chunk_opt.has_value()) {
		Ref<Chunk> chunk = chunk_opt.value();

		Engine::get().get_thread_pool().async([this, req, chunk] {
		    send_chunk(req.peer, chunk);
		});
	    } else {
		// TODO: chunk is loaded but requested by a client, so we load the chunk and send it when its ready.
		//       This will require to split chunks in two: chunks loaded or visible chunks.
	    }
	}
	m_load_requests.clear();
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

	    Engine::get().get_thread_pool().async([this, pos] { load_one_chunk(pos); });
        }

    std::lock_guard<std::mutex> lock(m_dims[0].mutex());
    for (const auto& [pos, chunk] : m_dims[0].m_chunks)
    {
        if (pos.x >= player_cx - m_load_distance && pos.x <= player_cx + m_load_distance && pos.z >= player_cz - m_load_distance && pos.z <= player_cz + m_load_distance)
        {
            continue;
        }

	Engine::get().get_thread_pool().async([this, pos] { unload_one_chunk(pos); });
    }
}

void World::request_load_around()
{
    const glm::vec3 player_pos = m_camera->get_global_transform().position();
    int64_t player_cx = int64_t(player_pos.x / 16);
    int64_t player_cz = int64_t(player_pos.z / 16);

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++) {
	for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++) {
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
	    
	    RequestChunkPacket p;
	    p.x = x;
	    p.z = z;
	    Engine::get().connection().send(Engine::get().connection().create_packet(p));
	}
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
    if (Engine::get().is_save_disabled()) {
	return Result<void>();
    }
    
    String path = format("{}/saves/{}/DIM0/{}${}/", Filesystem::get_data_directory(), m_name, chunk->x(), chunk->z());
    TRY(Filesystem::make_dirs(path));

    path.append("blocks.dat");
    File file = TRY(Filesystem::open_file(path, true));

    std::vector<uint8_t> compressed_data;
    deflate_data((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count, compressed_data);

    TRY(file.writer().write_raw(compressed_data.data(), compressed_data.size()));

    file.close();

    path = format("{}/saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, chunk->x(), chunk->z());
    file = TRY(Filesystem::open_file(path, true));

    BufferWriter writer;
    write_tags(writer, chunk);

    std::vector<uint8_t> tags_data;
    deflate_data(writer.buffer().data(), writer.buffer().size(), tags_data);

    TRY(file.writer().write_raw(tags_data.data(), tags_data.size()));
    
    //FileWriter writer = file.writer();
    //write_tags(writer, chunk);
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
    // TODO: save to disk

    return Result<void>();
}

Result<void> World::save_player(const Ref<Player>& player)
{
    String path = format("{}saves/{}/players/", Filesystem::get_data_directory(), m_name);
    TRY(Filesystem::make_dirs(path));

    path.append(format("{}.dat", player->get_username()));

    EntitySerializer serializer;
    serializer.set("position", player->get_position());
    serializer.set("rotation", player->get_rotation());
    player->save(serializer);

    TRY(serializer.save(path));

    return Result<void>();
}

void World::load_player(const StringView& username, Ref<Player>& player)
{
    String path = format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), get_name(), username);

    EntitySerializer serializer;
    EXPECT(serializer.load(path));

    glm::vec3 position = serializer.get<glm::vec3>("position").value_or(get_spawn_position());
    glm::quat rotation = serializer.get<glm::quat>("rotation").value_or({});

    player->set_position(position);
    player->set_rotation(rotation);
    player->load(serializer);
}

void World::send_chunk(ENetPeer *peer, const Ref<Chunk>& chunk) const
{
    std::vector<uint8_t> blocks_data;
    deflate_data((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count, blocks_data);

    BufferWriter writer;
    write_tags(writer, chunk);
    std::vector<uint8_t> tags_data;
    deflate_data(writer.buffer().data(), writer.buffer().size(), tags_data);

    ChunkDataPacket p;
    p.x = chunk->x();
    p.z = chunk->z();

    p.blocks.resize(blocks_data.size());
    memcpy(p.blocks.data(), blocks_data.data(), blocks_data.size());

    p.tags.resize(tags_data.size());
    memcpy(p.tags.data(), tags_data.data(), tags_data.size());

    Engine::get().connection().send(peer, Engine::get().connection().create_packet(p));
}

void World::receive_chunk(const ChunkDataPacket& p)
{
    Dimension& dimension = get_dimension(World::overworld);
    bool has_chunk = dimension.has_chunk(p.x, p.z);
    
    Ref<Chunk> chunk;
    if (has_chunk) {
	chunk = dimension.get_chunk(p.x, p.z).value();
	// TODO: maybe we need a mutex to modify a chunk, for now it only creates new one.
    } else {
	chunk = newref<Chunk>(&dimension, p.x, p.z);
    }

    std::vector<uint8_t> blocks_data;
    inflate_data(p.blocks.data(), p.blocks.size(), blocks_data);
    if (blocks_data.size() != sizeof(BlockState) * Chunk::block_count) {
	debug("received bad or corrupted blocks data for {} {}", p.x, p.z);
	return;
    }
    memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

    std::vector<uint8_t> tags_data;
    inflate_data(p.tags.data(), p.tags.size(), tags_data);

    // debug("tags received = {}", tags_data.size());

    BufferReader reader(tags_data.data(), tags_data.size());
    read_tags(reader, chunk);
    
    // for (size_t i = 0; i < Chunk::slice_count; i++) {
    //  	EXPECT(chunk->build_simple_mesh(i));
    // 	EXPECT(chunk->build_water_mesh(i));
    // }

    if (!has_chunk) {
    	dimension.add_chunk(p.x, p.z, chunk);
    }
}

void World::deferred_receive_chunk(const ChunkDataPacket& p)
{
    // Maybe I'm dumb and I don't know anything but using `[&]` creates segfaults, but manually specifying captures don't.
    Engine::get().get_thread_pool().async([this, p]() { receive_chunk(p); });
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
    if (!Engine::get().is_save_disabled() && Filesystem::exists(path))
    {
        chunk = newref<Chunk>(&m_dims[0], pos.x, pos.z);

        LocalVector<char> data;
        // TODO: how to handle errors from loading chunks ?
        File file = EXPECT(Filesystem::open_file(path));
        EXPECT(file.reader().read_to_buffer(data));
        file.close();

	std::vector<uint8_t> blocks_data;
	inflate_data((uint8_t *)data.data(), data.size(), blocks_data);

	assert(blocks_data.size() == sizeof(BlockState) * Chunk::block_count);
	memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

        String path = format("{}saves/{}/DIM0/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, pos.x, pos.z);
        if (Filesystem::exists(path))
        {
            File file = EXPECT(Filesystem::open_file(path));
	    LocalVector<char> tags_compressed_data;
	    EXPECT(file.reader().read_to_buffer(tags_compressed_data));
	    file.close();

	    std::vector<uint8_t> tags_data;
	    inflate_data((uint8_t *)tags_compressed_data.data(), tags_compressed_data.size(), tags_data);

	    BufferReader reader(tags_data.data(), tags_data.size());
	    read_tags(reader, chunk);
        }

        // for (size_t i = 0; i < Chunk::slice_count; i++)
        // {
        //     EXPECT(chunk->build_simple_mesh(i));
        //     EXPECT(chunk->build_water_mesh(i));
        // }
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

#define DEFLATE_BUFFER_SIZE (sizeof(BlockState) * 1024)
#define INFLATE_BUFFER_SIZE (sizeof(BlockState) * 1024)

void World::deflate_data(const uint8_t *data, size_t size, std::vector<uint8_t>& compressed_data) const
{
    uint8_t tmp[DEFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = (uint8_t *)data;
    strm.avail_in = size;
    strm.next_out = tmp;
    strm.avail_out = DEFLATE_BUFFER_SIZE;
    deflateInit(&strm, Z_BEST_COMPRESSION);

    while(strm.avail_in != 0) {
	int res = deflate(&strm, Z_NO_FLUSH);
	assert(res == Z_OK);

	if (strm.avail_out == 0) {
	    compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
	    strm.next_out = tmp;
	    strm.avail_out = DEFLATE_BUFFER_SIZE;
	}
    }

    int deflate_res = Z_OK;
    while (deflate_res == Z_OK) {
	if (strm.avail_out == 0) {
	    compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE);
	    strm.next_out = tmp;
	    strm.avail_out = DEFLATE_BUFFER_SIZE;
	}
	deflate_res = deflate(&strm, Z_FINISH);
    }

    assert(deflate_res == Z_STREAM_END);
    compressed_data.insert(compressed_data.end(), tmp, tmp + DEFLATE_BUFFER_SIZE - strm.avail_out);
    deflateEnd(&strm);
}

void World::inflate_data(const uint8_t *compressed_data, size_t compressed_data_size, std::vector<uint8_t>& uncompressed_data) const
{
    uint8_t tmp[INFLATE_BUFFER_SIZE];

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = (uint8_t *)compressed_data;
    strm.avail_in = compressed_data_size;
    strm.next_out = tmp;
    strm.avail_out = INFLATE_BUFFER_SIZE;
    inflateInit(&strm);

    while(strm.avail_in != 0) {
	int res = inflate(&strm, Z_NO_FLUSH);
	assert(res >= 0);

	if (strm.avail_out == 0) {
	    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE);
	    strm.next_out = tmp;
	    strm.avail_out = INFLATE_BUFFER_SIZE;
	}
    }

    uncompressed_data.insert(uncompressed_data.end(), tmp, tmp + INFLATE_BUFFER_SIZE - strm.avail_out);
    inflateEnd(&strm);
    
    // TODO: Obsiously we don't want to crash on errors.
}

void World::write_tags(Writer& writer, const Ref<Chunk>& chunk) const
{
    Map<int64_t, Map<String, Variant>> tags;
    for (const auto& [key, value] : chunk->m_tags)
    {
        Map<String, Variant> tags2;
        for (const auto& [_, key2, value2] : value.tags)
            tags2.put(key2, value2);
        tags.put(key, tags2);
    }
    EXPECT(writer.write_variant(Variant(tags)));
}

void World::read_tags(Reader& reader, Ref<Chunk>& chunk) const
{
    Option<Variant> variant = EXPECT(reader.read_variant());
    if (variant.has_value()) {
        Map<int64_t, Map<String, Variant>> tags = variant.value().to_map<int64_t, Map<String, Variant>>();
        for (const auto& [key, value] : tags) {
            BlockTags btags;
            for (const auto& [key2, value2] : value)
                btags.tags.put(key2, value2);
            chunk->m_tags.put(key, btags);
        }
    }
}

void World::request_chunk(ENetPeer *peer, int dimension, int64_t x, int64_t z)
{
    m_load_requests.append(ChunkLoadRequest(peer, dimension, x, z));
}
