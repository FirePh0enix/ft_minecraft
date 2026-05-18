#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Ref.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Pass/Flat.hpp"
#include "World/Pass/Overworld.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <thread>

// https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
bool ray_intersect_aabb(const Ray& ray, const AABB& aabb, float& t_min)
{
    glm::vec3 inv_dir = 1.0f / ray.dir();
    glm::vec3 t0s = (aabb.min - ray.origin()) * inv_dir;
    glm::vec3 t1s = (aabb.max - ray.origin()) * inv_dir;

    glm::vec3 tsmaller = glm::min(t0s, t1s);
    glm::vec3 tbigger = glm::max(t0s, t1s);

    t_min = std::max({tsmaller.x, tsmaller.y, tsmaller.z});
    float t_max = std::min({tbigger.x, tbigger.y, tbigger.z});

    if (t_max < 0.0f)
        return false;

    return t_min <= t_max;
}

World::World(uint64_t seed, int type)
    : m_seed(seed), m_generation_thread_pool(std::thread::hardware_concurrency() - 1)
{
    if (type == WorldPresetFlat)
    {
        // EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatBiomePass>())));
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatSurfacePass>())));
    }
    else if (type == WorldPresetNormal)
    {
        // EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldBiomePass>())));
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldSurfacePass>())));
    }

    find_safe_spawn();

    // Wait for the spawn chunk to be loaded or else the player might fall through the floor.
    int64_t chunk_x = int64_t(m_spawn_position.x) >= 0 ? (int64_t(m_spawn_position.x) / 16) : (int64_t(m_spawn_position.x) / 16 - 1);
    int64_t chunk_z = int64_t(m_spawn_position.z) >= 0 ? (int64_t(m_spawn_position.z) / 16) : (int64_t(m_spawn_position.z) / 16 - 1);
    load_one_chunk(ChunkPos(chunk_x, chunk_z));
}

World::World(uint64_t seed)
    : m_seed(seed)
{
}

void World::find_safe_spawn()
{
    srand(0);
    glm::vec3 water_spawn_position;
    uint16_t water_id = Engine::get().block_registry().get_block_id("water");
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

Result<Ref<World>> World::create_proxy(uint64_t seed)
{
    Ref<World> world = EXPECT(newref<World>(seed));
    world->m_seed = seed;
    world->m_proxy = true;
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
        // else
        //     remove_entity(entity->m_dimension, entity->id());
    }

    if (!m_proxy)
    {
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
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return BlockState();
    }

    Ref<Chunk> chunk = chunk_value.value();
    const int64_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const int64_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    return chunk->get_block(chunk_local_x, y, chunk_local_z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const int64_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const int64_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    chunk->set_block(chunk_local_x, y, chunk_local_z, state);

    if (chunk_local_x == 0)
    {
        chunk_value = get_chunk(chunk_x - 1, chunk_z);
        if (chunk_value.has_value())
            chunk_value.value()->set_block(16, y, chunk_local_z, state);
    }
    else if (chunk_local_x == 15)
    {
        chunk_value = get_chunk(chunk_x + 1, chunk_z);
        if (chunk_value.has_value())
            chunk_value.value()->set_block(-1, y, chunk_local_z, state);
    }
    if (chunk_local_z == 0)
    {
        chunk_value = get_chunk(chunk_x, chunk_z - 1);
        if (chunk_value.has_value())
            chunk_value.value()->set_block(chunk_local_x, y, 16, state);
    }
    else if (chunk_local_z == 15)
    {
        chunk_value = get_chunk(chunk_x, chunk_z + 1);
        if (chunk_value.has_value())
            chunk_value.value()->set_block(chunk_local_x, y, -1, state);
    }
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

    for (const Ref<Entity>& e : m_dims[overworld].get_entities())
    {
        float t = 0.0f;
        if (ray_intersect_aabb(ray, e->m_aabb, t) && t < t_min)
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
        if (!get_block_state(ipos.x, ipos.y, ipos.z).is_air() && ray_intersect_aabb(ray, AABB(-glm::vec3(0.5), glm::vec3(0.5)).translate(pos), t) && t < t_min)
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
        return true;
    }
    return false;
}

void World::load_one_chunk(ChunkPos pos)
{
    Result<Ref<Chunk>> result = m_dims[0].generate_chunk(pos.x, pos.z);
    if (result.has_error())
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_dims[0].mutex());
        Ref<Chunk> chunk = result.value();
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
