#include "World/World.hpp"

#include "AABB.hpp"
#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"
#include "World/Settings.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <format>
#include <limits>
#include <memory>
#include <mutex>

// https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
static bool ray_intersect_aabb(const Ray& ray, const AABBd& aabb, double& t_min, glm::dvec3& normal)
{
    glm::dvec3 inv_dir = 1.0 / ray.dir();
    glm::dvec3 t0s = (aabb.min - ray.origin()) * inv_dir;
    glm::dvec3 t1s = (aabb.max - ray.origin()) * inv_dir;

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
    : m_dims{Dimension(0), Dimension(1)}
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

Result<std::shared_ptr<World>> World::create(std::string name, uint64_t seed, int type)
{
    std::shared_ptr<World> world = std::make_shared<World>();
    world->m_seed = seed;
    world->m_name = name;

    // TODO: find a common place to put this.
    world->m_dims[overworld].m_world = world.get();
    world->m_dims[overworld].m_gen = std::make_shared<OverworldGen>(WorldSettings{});
    world->m_dims[underworld].m_world = world.get();
    world->m_dims[underworld].m_gen = std::make_shared<UnderworldGen>(WorldSettings{});

    // world->find_safe_spawn();

    if (!Engine::get().is_save_disabled())
    {
        std::string path = std::format("{}saves/{}/", Filesystem::get_data_directory(), name);
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

Result<std::shared_ptr<World>> World::create_proxy(uint64_t seed)
{
    std::shared_ptr<World> world = std::make_shared<World>();
    world->m_seed = seed;
    world->m_proxy = true;
    world->m_dims[overworld].m_world = world.get();
    return world;
}

Result<std::shared_ptr<World>> World::load(std::string name)
{
    std::shared_ptr<World> world = std::make_shared<World>();

    if (!Engine::get().is_save_disabled())
    {
        std::string path = std::format("{}saves/{}/info.dat", Filesystem::get_data_directory(), name);
        File file = TRY(Filesystem::open_file(path));

        WorldSaveInfo wi{};
        TRY(file.reader().read_raw(&wi, sizeof(WorldSaveInfo)));
        file.close();

        world->m_seed = wi.seed;
        world->m_spawn_position = wi.spawn_position;
    }

    world->m_name = name;

    world->m_dims[overworld].m_world = world.get();
    world->m_dims[overworld].m_gen = std::make_shared<OverworldGen>(WorldSettings{});
    world->m_dims[underworld].m_world = world.get();
    world->m_dims[underworld].m_gen = std::make_shared<UnderworldGen>(WorldSettings{});

    return world;
}

World::~World()
{
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

void World::tick(float delta)
{
    // int dimension = m_player->get_dimension();

    // tick_dimension(delta, dimension);
    tick_dimension(delta, overworld);
    // tick_dimension(delta, underworld);

    m_debug_display.update(delta);
}

void World::tick_dimension(float delta, int dimension)
{
    ZoneScoped;

    for (std::shared_ptr<Entity> entity : m_dims[dimension].get_entities())
    {
        if (entity->is_active())
            entity->recurse_tick(delta);
        else
            remove_entity(entity->m_dimension, entity);
    }

    for (std::shared_ptr<Entity> entity : m_dims[dimension].m_entities_to_remove)
        m_dims[dimension].m_entities.erase(std::find(m_dims[dimension].m_entities.begin(), m_dims[dimension].m_entities.end(), entity));
    for (std::shared_ptr<Entity> entity : m_dims[dimension].m_entities_to_add)
        m_dims[dimension].m_entities.push_back(entity);

    if (!m_proxy)
    {
        for (auto& [pos, chunk] : m_dims[dimension].m_chunks)
        {
            if (chunk->is_modified())
                EXPECT(save_chunk(chunk, dimension));
            chunk->clear_modified();
        }

        // TODO: Don't save every players each frames.
        for (const std::shared_ptr<Entity>& entity : m_dims[dimension].get_entities())
        {
            if (std::shared_ptr<Player> player = std::dynamic_pointer_cast<Player>(entity))
                EXPECT(save_player(player));
        }

        load_around_player(dimension);
    }
    else if (Engine::get().is_online() && !Engine::get().is_server())
    {
        request_load_around(dimension);
    }

    // Flush all new chunks.
    std::set<ChunkPos> chunk_modified;

    std::shared_ptr<PreLoadedChunk> pregen_chunk;
    while (m_dims[dimension].m_pregen_chunks_lockless.try_pop(pregen_chunk))
    {
        m_dims[dimension].m_preloaded_chunks[pregen_chunk->pos] = pregen_chunk;
        m_dims[dimension].m_scheduler.m_pregen_count.fetch_sub(1);
    }

    Chunk *chunk = nullptr;
    while (m_dims[dimension].m_chunks_lockless.try_pop(chunk))
    {
        const ChunkPos pos = chunk->pos();
        m_dims[dimension].m_chunks_loading_queue.erase(pos);
        if (m_dims[dimension].m_chunks.contains(pos))
            continue;
        m_dims[dimension].m_chunks[pos] = chunk;
        chunk_modified.insert(pos);
        add_neighbour_chunk(pos, chunk_modified);
    }

    MeshRebuildResult result;
    while (m_dims[dimension].m_mesh_queue_lockless.try_pop(result))
    {
        Chunk *chunk = m_dims[dimension].m_chunks[result.pos];
        chunk->get_slices()[result.slice_index].mesh = result.mesh;
        chunk->get_slices()[result.slice_index].water_mesh = result.water_mesh;
    }

    chunk = nullptr;
    while (m_dims[dimension].m_chunks_unload_queue.try_pop(chunk))
    {
        const ChunkPos pos = chunk->pos();
        m_dims[dimension].m_chunks.erase(chunk->pos());
        m_dims[dimension].free_chunk(chunk);
        chunk_modified.insert(pos);
        add_neighbour_chunk(pos, chunk_modified);
    }

    for (ChunkPos pos : chunk_modified)
    {
        m_dims[dimension].queue_rebuild(pos);
    }

    if (!m_proxy && Engine::get().is_online() && Engine::get().is_server())
    {
        for (std::shared_ptr<Entity> entity : m_dims[dimension].get_entities())
        {
            UpdateEntityPacket p{};
            p.id = entity->id();
            p.position = entity->get_transform().position();
            p.rotation = entity->get_transform().rotation();
            Engine::get().connection().broadcast(Engine::get().connection().create_packet(p));
        }

        for (const ChunkLoadRequest& req : m_load_requests)
        {
            std::optional<Chunk *> chunk_opt = get_dimension(req.dimension).get_chunk(req.x, req.z);
            if (chunk_opt.has_value())
            {
                Chunk *chunk = chunk_opt.value();
                Engine::get().get_thread_pool().async([this, req, chunk]
                                                      { send_chunk(req.peer, chunk); });
            }
            else
            {
                // TODO: chunk is loaded but requested by a client, so we load the chunk and send it when its ready.
                //       This will require to split chunks in two: chunks loaded or visible chunks.
            }
        }
        m_load_requests.clear();
    }

    m_dims[dimension].m_entities_to_remove.clear();
    m_dims[dimension].m_entities_to_add.clear();

    std::shared_ptr<Camera> camera = m_player->get_camera();

    {
        ZoneScopedN("calc visible chunks");

        m_dims[dimension].m_visible_chunks.resize(0);
        for (const auto& [key, chunk] : m_dims[dimension].m_chunks)
        {
            ZoneScopedN("one chunk");

            ChunkPos pos = chunk->pos();
            AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                             .translate(glm::dvec3((double)pos.x * Chunk::width + Chunk::width / 2.0, double(Chunk::height) / 2.0, (double)pos.z * Chunk::width + Chunk::width / 2.0) - camera->get_global_transform().position());

            if (!camera->frustum().contains(aabb))
                continue;

            for (size_t i = 0; i < Chunk::slice_count; i++)
            {
                ChunkPos pos = chunk->pos();
                AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
                                 .translate(glm::dvec3((double)pos.x * Chunk::width + Chunk::width / 2.0, (double)i * Chunk::width + Chunk::width / 2.0, (double)pos.z * Chunk::width + Chunk::width / 2.0) - camera->get_global_transform().position());

                if (!camera->frustum().contains(aabb) || (chunk->get_slices()[i].mesh == nullptr && chunk->get_slices()[i].water_mesh == nullptr))
                    continue;

                m_dims[dimension].m_visible_chunks.push_back(RenderableChunk(chunk, i));
            }
        }
    }

    m_dims[dimension].m_sun_visible_chunks.resize(0);
    for (const auto& [key, chunk] : m_dims[dimension].m_chunks)
    {
        ChunkPos pos = chunk->pos();
        AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                         .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, float(Chunk::height) / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

        if (!m_dims[dimension].m_sun_frustum.contains(aabb))
            continue;

        for (size_t i = 0; i < Chunk::slice_count; i++)
        {
            ChunkPos pos = chunk->pos();
            AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::width / 2.0, Chunk::width / 2))
                             .translate(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)i * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0));

            std::shared_ptr<Mesh> mesh = chunk->get_slices()[i].mesh;
            if (!m_dims[dimension].m_sun_frustum.contains(aabb) || mesh == nullptr)
                continue;

            m_dims[dimension].m_sun_visible_chunks.push_back(RenderableChunk(chunk, i));
        }
    }
}

BlockState World::get_block_state(int dimension, int64_t x, int64_t y, int64_t z) const
{
    return m_dims[dimension].get_block(x, y, z);
}

void World::set_block_state(int dimension, int64_t x, int64_t y, int64_t z, BlockState state)
{
    m_dims[dimension].set_block(x, y, z, state);
}

std::optional<Chunk *> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[overworld].get_chunk(x, z);
}

std::optional<Chunk *> World::get_chunk(int64_t x, int64_t z)
{
    return m_dims[overworld].get_chunk(x, z);
}

void World::load_around_player(int dimension)
{
    const glm::vec3 camera_pos = m_player->get_global_transform().position();
    m_dims[dimension].load((int64_t)std::round(camera_pos.x), (int64_t)std::round(camera_pos.y), (int64_t)std::round(camera_pos.z), m_load_distance);
}

void World::request_load_around(int dimension)
{
    const glm::vec3 player_pos = m_player->get_global_transform().position();
    int64_t player_cx = int64_t(player_pos.x / 16);
    int64_t player_cz = int64_t(player_pos.z / 16);

    for (int64_t cx = -m_load_distance; cx <= m_load_distance; cx++)
    {
        for (int64_t cz = -m_load_distance; cz <= m_load_distance; cz++)
        {
            int64_t x = player_cx + cx;
            int64_t z = player_cz + cz;
            ChunkPos pos(x, z);

            // {
            //     std::lock_guard<std::mutex> lock(m_dims[dimension].m_chunk_mutex);
            //     if (m_dims[dimension].has_chunk(x, z) || m_dims[dimension].m_chunks_to_flush.contains(pos))
            //         continue;
            // }

            // {
            //     std::lock_guard<std::mutex> lock(m_dims[dimension].m_chunk_loading_mutex);
            //     if (m_dims[dimension].m_chunk_loading_queue.contains(pos))
            //         continue;

            //     m_dims[dimension].m_chunk_loading_queue.insert(pos);
            // }

            // FIXME: don't request over and over

            RequestChunkPacket p{};
            p.x = x;
            p.z = z;
            // TODO: dimension
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

bool World::raycast(int dimension, const Ray& ray, float range, RaycastResult& result, const Entity *ignore)
{
    bool hit = false;
    bool is_entiy = false;
    double t_min = std::numeric_limits<double>::infinity();
    glm::i64vec3 block_pos;
    std::shared_ptr<Entity> entity;
    glm::dvec3 normal;

    for (const std::shared_ptr<Entity>& e : m_dims[dimension].get_entities())
    {
        if (e.get() == ignore)
            continue;

        AABB world_aabb = e->get_aabb().translate(e->get_global_transform().position());
        double t = 0.0f;
        glm::dvec3 normal;
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
        double t;
        if (!get_block_state(dimension, ipos.x, ipos.y, ipos.z).is_air() && ray_intersect_aabb(ray, AABBd(-glm::dvec3(0.5), glm::dvec3(0.5)).translate(pos), t, normal) && t < t_min)
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

void World::break_block(int dimension, int64_t x, int64_t y, int64_t z)
{
    BlockState state = get_block_state(dimension, x, y, z);
    set_block_state(dimension, x, y, z, BlockState());

    std::optional<Id<Item>> item_opt = Engine::get().registry().to_item(Engine::get().registry().from_runtime_id(state.id));
    if (!item_opt.has_value())
        return;

    std::shared_ptr<ItemEntity> item_entity = std::make_shared<ItemEntity>(item_opt.value());
    item_entity->set_position(glm::vec3(x, y, z) + glm::vec3(rand_float(-0.6, 0.6), 0, rand_float(-0.6, 0.6)));

    add_entity(World::overworld, item_entity);
}

Result<void> World::save_chunk(Chunk *chunk, int dimension)
{
    if (Engine::get().is_save_disabled())
    {
        return Result<void>();
    }

    std::string path = std::format("{}/saves/{}/DIM{}/{}${}/", Filesystem::get_data_directory(), m_name, dimension, chunk->x(), chunk->z());
    TRY(Filesystem::make_dirs(path));

    path.append("blocks.dat");
    File file = TRY(Filesystem::open_file(path, true));

    std::vector<uint8_t> compressed_data;
    TRY(ZLib::deflate(std::as_bytes(std::span((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count)), compressed_data));

    TRY(file.writer().write_raw(compressed_data.data(), compressed_data.size()));

    file.close();

    path = std::format("{}/saves/{}/DIM{}/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, dimension, chunk->x(), chunk->z());
    file = TRY(Filesystem::open_file(path, true));

    BufferWriter writer;
    Dimension::write_tags(writer, chunk);

    std::vector<uint8_t> tags_data;
    TRY(ZLib::deflate(std::as_bytes(writer.buffer()), tags_data));

    TRY(file.writer().write_raw(tags_data.data(), tags_data.size()));

    // FileWriter writer = file.writer();
    // write_tags(writer, chunk);
    file.close();

    return Result<void>();
}

Result<void> World::save_entity(const std::shared_ptr<Entity>& entity)
{
    int64_t cx = chunk_index(int64_t(entity->get_position().x));
    int64_t cz = chunk_index(int64_t(entity->get_position().z));

    std::string path = std::format("{}saves/{}/DIM0/{}${}/entities/", Filesystem::get_data_directory(), m_name, cx, cz);
    TRY(Filesystem::make_dirs(path));

    EntitySerializer serializer;
    serializer.set("position", entity->get_position());
    serializer.set("rotation", entity->get_rotation());
    TRY(entity->save(serializer));

    path.append("");
    // TODO: save to disk

    return Result<void>();
}

Result<void> World::save_player(const std::shared_ptr<Player>& player)
{
    std::string path = std::format("{}saves/{}/players/", Filesystem::get_data_directory(), m_name);
    TRY(Filesystem::make_dirs(path));

    path.append(std::format("{}.dat", player->get_username()));

    EntitySerializer serializer;
    serializer.set("position", player->get_position());
    serializer.set("rotation", player->get_rotation());
    TRY(player->save(serializer));

    TRY(serializer.save(path));

    return Result<void>();
}

bool World::load_player(std::string_view username, std::shared_ptr<Player>& player)
{
    std::string path = std::format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), get_name(), username);

    EntitySerializer serializer;
    Result<void> result = serializer.load(path);

    if (result.has_error())
    {
        error("player data is corrupted");
        // TODO: maybe copy corrupted data to a `.backup` file before writing over the data.
        return false;
    }

    glm::dvec3 position = serializer.get<glm::dvec3>("position").value_or(get_spawn_position());
    glm::dquat rotation = serializer.get<glm::dquat>("rotation").value_or(glm::identity<glm::dquat>());

    player->set_position(position);
    player->set_rotation(rotation);

    result = player->load(serializer);
    if (result.has_error())
    {
        error("player data is corrupted");
        return false;
    }

    return true;
}

void World::send_chunk(ENetPeer *peer, const Chunk *chunk) const
{
    std::vector<uint8_t> blocks_data;
    EXPECT(ZLib::deflate(std::as_bytes(std::span((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count)), blocks_data));

    BufferWriter writer;
    Dimension::write_tags(writer, chunk);
    std::vector<uint8_t> tags_data;
    EXPECT(ZLib::deflate(std::as_bytes(writer.buffer()), tags_data));

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

    Chunk *chunk;
    if (has_chunk)
    {
        chunk = dimension.get_chunk(p.x, p.z).value();
        // TODO: maybe we need a mutex to modify a chunk, for now it only creates new one.
    }
    else
    {
        chunk = dimension.alloc_chunk(p.x, p.z).value();
    }

    std::vector<uint8_t> blocks_data;
    EXPECT(ZLib::inflate(std::as_bytes(std::span(p.blocks)), blocks_data));
    if (blocks_data.size() != sizeof(BlockState) * Chunk::block_count)
    {
        debug("received bad or corrupted blocks data for {} {}", p.x, p.z);
        return;
    }
    memcpy(chunk->get_blocks(), blocks_data.data(), blocks_data.size());

    std::vector<uint8_t> tags_data;
    EXPECT(ZLib::inflate(std::as_bytes(std::span(p.tags)), tags_data));

    // debug("tags received = {}", tags_data.size());

    BufferReader reader(tags_data.data(), tags_data.size());
    Dimension::read_tags(reader, chunk);

    // for (size_t i = 0; i < Chunk::slice_count; i++) {
    //  	EXPECT(chunk->build_simple_mesh(i));
    // 	EXPECT(chunk->build_water_mesh(i));
    // }

    if (!has_chunk)
    {
        // TODO: Is there a reason to lock anything here ?
        dimension.m_chunks[chunk->pos()] = chunk;
    }
}

void World::queue_receive_chunk(const ChunkDataPacket& p)
{
    // Maybe I'm dumb and I don't know anything but using `[&]` creates segfaults, but manually specifying captures don't.
    Engine::get().get_thread_pool().async([this, p]()
                                          { receive_chunk(p); });
}

bool World::is_player_saved(std::string_view name) const
{
    std::string path = std::format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), m_name, name);
    return Filesystem::exists(path);
}

void World::request_chunk(ENetPeer *peer, int dimension, int64_t x, int64_t z)
{
    m_load_requests.push_back(ChunkLoadRequest(peer, dimension, x, z));
}
