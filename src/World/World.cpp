#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Ref.hpp"
#include "Entity/Entity.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Pass/Flat.hpp"
#include "World/Pass/Overworld.hpp"

#include <mutex>

// https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
bool ray_intersect_aabb(const Ray& ray, const AABB& aabb, float& t_min)
{

    glm::vec3 inv_dir = 1.0f / ray.dir();
    glm::vec3 t0s = (aabb.min() - ray.origin()) * inv_dir;
    glm::vec3 t1s = (aabb.max() - ray.origin()) * inv_dir;

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
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatBiomePass>())));
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<FlatSurfacePass>())));
    }
    else if (type == WorldPresetNormal)
    {
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldBiomePass>())));
        EXPECT(m_dims[overworld].m_generation_passes.append(EXPECT(newref<OverworldSurfacePass>())));
    }
}

World::World(uint64_t seed)
    : m_seed(seed)
{
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

std::optional<RaycastResult> World::raycast(const Ray& ray, float range)
{
    const glm::vec3 ro = ray.origin();
    const glm::vec3 rd = ray.dir();

    // avoid zero direction
    if (ro == glm::vec3())
        return std::nullopt;

    // current voxel coordinates
    int64_t vx = (int)std::floor(ro.x);
    int64_t vy = (int)std::floor(ro.y);
    int64_t vz = (int)std::floor(ro.z);

    // if starting exactly on integer boundary and ray goes negative, move to neighbor voxel
    adjust_on_boundary(ro.x, vx, rd.x);
    adjust_on_boundary(ro.y, vy, rd.y);
    adjust_on_boundary(ro.z, vz, rd.z);

    if (vy >= Chunk::height)
        return std::nullopt;

    // If starting inside a solid voxel, report immediate hit at t=0
    if (!get_block_state(vx, vy, vz).is_air())
    {
        // Determine closest face? common choice: return entry face as opposite of ray direction
        Face face;
        if (std::abs(rd.x) >= std::abs(rd.y) && std::abs(rd.x) >= std::abs(rd.z))
        {
            face = (rd.x > 0.0) ? Face::NegX : Face::PosX;
        }
        else if (std::abs(rd.y) >= std::abs(rd.x) && std::abs(rd.y) >= std::abs(rd.z))
        {
            face = (rd.y > 0.0) ? Face::NegY : Face::PosY;
        }
        else
        {
            face = (rd.z > 0.0) ? Face::NegZ : Face::PosZ;
        }
        return RaycastResult{vx, vy, vz, face, ro, 0.0};
    }

    // step direction: +1 or -1 per axis
    int64_t step_x = (rd.x > 0.0) ? 1 : (rd.x < 0.0) ? -1
                                                     : 0;
    int64_t step_y = (rd.y > 0.0) ? 1 : (rd.y < 0.0) ? -1
                                                     : 0;
    int64_t step_z = (rd.z > 0.0) ? 1 : (rd.z < 0.0) ? -1
                                                     : 0;

    // tMax: distance along ray to the first voxel boundary on each axis
    float tmax_x, tmax_y, tmax_z;
    // tDelta: distance along ray between subsequent crossings of voxel boundaries
    float tdelta_x, tdelta_y, tdelta_z;

    auto safe_div = [](float a, float b) -> float
    { return (b == 0.0) ? INFINITY : a / b; };

    // compute initial tMax for X
    if (step_x != 0)
    {
        float nextBoundaryX = float(vx + (step_x > 0 ? 1 : 0));
        tmax_x = safe_div(nextBoundaryX - ro.x, rd.x);
        tdelta_x = std::abs(safe_div(1.0, rd.x));
    }
    else
    {
        tmax_x = INFINITY;
        tdelta_x = INFINITY;
    }

    if (step_y != 0)
    {
        float nextBoundaryY = float(vy + (step_y > 0 ? 1 : 0));
        tmax_y = safe_div(nextBoundaryY - ro.y, rd.y);
        tdelta_y = std::abs(safe_div(1.0, rd.y));
    }
    else
    {
        tmax_y = INFINITY;
        tdelta_y = INFINITY;
    }

    if (step_z != 0)
    {
        float nextBoundaryZ = float(vz + (step_z > 0 ? 1 : 0));
        tmax_z = safe_div(nextBoundaryZ - ro.z, rd.z);
        tdelta_z = std::abs(safe_div(1.0, rd.z));
    }
    else
    {
        tmax_z = INFINITY;
        tdelta_z = INFINITY;
    }

    float t = 0.0;
    const int max_steps = 100000; // safety cap
    for (int i = 0; i < max_steps && t <= range; ++i)
    {
        // advance to next voxel boundary
        if (tmax_x < tmax_y)
        {
            if (tmax_x < tmax_z)
            {
                // step X
                vx += step_x;
                t = tmax_x;
                tmax_x += tdelta_x;
                // entering through ±X face
                Face face = (step_x > 0) ? Face::NegX : Face::PosX;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::dvec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
            else
            {
                // step Z
                vz += step_z;
                t = tmax_z;
                tmax_z += tdelta_z;
                Face face = (step_z > 0) ? Face::NegZ : Face::PosZ;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::dvec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
        }
        else
        {
            if (tmax_y < tmax_z)
            {
                // step Y
                vy += step_y;
                t = tmax_y;
                tmax_y += tdelta_y;
                Face face = (step_y > 0) ? Face::NegY : Face::PosY;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::vec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
            else
            {
                // step Z
                vz += step_z;
                t = tmax_z;
                tmax_z += tdelta_z;
                Face face = (step_z > 0) ? Face::NegZ : Face::PosZ;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::vec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<EntityRaycastResult> World::raycast_entities(
    const Ray& ray, float range, Entity *self)
{
    EntityRaycastResult closest_hit{};
    float closest_distance = range;
    bool found = false;

    for (auto& entity_ref : m_dims[self->m_dimension].get_entities())
    {

        // TODO: Comparing entity id can be more efficient ? For now it seems like every entity has id 0.
        if (self == entity_ref.ptr())
            continue;

        float t;
        AABB world_aabb = entity_ref->get_aabb().translate(entity_ref->get_global_transform().position());

        if (ray_intersect_aabb(ray, world_aabb, t))
        {
            if (t >= 0.0f && t < closest_distance)
            {
                closest_distance = t;
                closest_hit.entity = entity_ref;
                closest_hit.hit_position = ray.at(t);
                closest_hit.distance = t;
                found = true;
            }
        }
    }

    if (found)
        return closest_hit;

    return std::nullopt;
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
