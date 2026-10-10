#include "World/World.hpp"

#include "AABB.hpp"
#include "Block/PortalPattern.hpp"
#include "Audio/AudioMixer.hpp"
#include "Core/Filesystem.hpp"
#include "Core/ZLib.hpp"
#include "Engine.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Item.hpp"
#include "Entity/Player.hpp"
#include "Network/Network.hpp"
#include "Network/LocalServer.hpp"
#include "World/Registry.hpp"
#include "Profiler.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"
#include "World/MobSpawner.hpp"
#include "World/Settings.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <expected>
#include <format>
#include <limits>
#include <memory>
#include <cmath>

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

World::World(AudioMixer& audio) : m_dims{Dimension(0), Dimension(1)}, m_audio(audio)
{
}

void World::find_safe_spawn()
{
    const int64_t size = 4;

    std::random_device dev;
    std::mt19937 rng(dev());
    rng.seed(m_seed);
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, size * 2 * 16);

    for (size_t i = 0; i < 32; i++)
    {
        int64_t x = int64_t(dist(rng)) - size * 16;
        int64_t z = int64_t(dist(rng)) - size * 16;

        std::shared_ptr<Chunk> chunk = get_chunk(chunk_index(x), chunk_index(z)).value_or(nullptr);
        if (chunk == nullptr)
            continue;

        int64_t lx = local_coords(x);
        int64_t lz = local_coords(z);

        for (int64_t y = Chunk::height - 3; y > 0; y--)
        {
            BlockState state0 = chunk->m_blocks[lx + y * 16 + lz * 16 * 256];
            BlockState state1 = chunk->m_blocks[lx + (y + 1) * 16 + lz * 16 * 256];
            BlockState state2 = chunk->m_blocks[lx + (y + 2) * 16 + lz * 16 * 256];

            if (!state0.is_air() && (state1.is_air() && !chunk->get_tag(lx + (y + 1) * 16 + lz * 16 * 256, "water").has_value()) && (state2.is_air() && !chunk->get_tag(lx + (y + 2) * 16 + lz * 16 * 256, "water").has_value()))
            {
                m_spawn_position = glm::dvec3(x, y, z) + glm::dvec3(0, 1.5, 0);

                if (!Engine::get().is_save_disabled())
                {
                    std::string path = std::format("{}saves/{}/", Filesystem::get_data_directory(), m_name);
                    EXPECT(Filesystem::make_dirs(path));
                    path.append("info.dat");
                    File file = EXPECT(Filesystem::open_file(path, true));

                    WorldSaveInfo wi{};
                    wi.seed = m_seed;
                    wi.type = WorldPresetType(0);
                    wi.spawn_position = get_spawn_position();
                    EXPECT(file.writer().write_raw(&wi, sizeof(WorldSaveInfo)));
                    file.close();
                }

                return;
            }
            else if (chunk->get_tag(lx + y * 16 + lz * 16 * 256, "water").has_value())
                break;
        }
    }
}

std::expected<std::shared_ptr<World>, Error> World::create(std::string name, uint64_t seed, int type, AudioMixer& audio)
{
    std::shared_ptr<World> world = std::make_shared<World>(audio);
    world->m_seed = seed;
    world->m_name = name;

    world->m_dims[overworld].m_world = world.get();
    world->m_dims[underworld].m_world = world.get();

    world->m_mob_spawner = std::make_unique<MobSpawner>(*world, overworld);

    if (!Engine::get().is_save_disabled())
    {
        std::string path = std::format("{}saves/{}/", Filesystem::get_data_directory(), name);
        TRY(Filesystem::make_dirs(path));
        path.append("info.dat");
        TRY_ASSIGN(File file, Filesystem::open_file(path, true));

        WorldSaveInfo wi{};
        wi.seed = seed;
        wi.type = WorldPresetType(type);
        wi.spawn_position = world->get_spawn_position();
        TRY(file.writer().write_raw(&wi, sizeof(WorldSaveInfo)));
        file.close();
    }

    return world;
}

std::expected<std::shared_ptr<World>, Error> World::create_proxy(uint64_t seed, AudioMixer& audio)
{
    std::shared_ptr<World> world = std::make_shared<World>(audio);
    world->m_seed = seed;
    world->m_proxy = true;
    world->m_dims[overworld].m_world = world.get();
    world->m_dims[underworld].m_world = world.get();
    world->m_mob_spawner = std::make_unique<MobSpawner>(*world, overworld);

    return world;
}

std::expected<std::shared_ptr<World>, Error> World::load(std::string name, AudioMixer& audio)
{
    std::shared_ptr<World> world = std::make_shared<World>(audio);

    if (!Engine::get().is_save_disabled())
    {
        std::string path = std::format("{}saves/{}/info.dat", Filesystem::get_data_directory(), name);
        TRY_ASSIGN(File file, Filesystem::open_file(path));

        WorldSaveInfo wi{};
        TRY(file.reader().read_raw(&wi, sizeof(WorldSaveInfo)));
        file.close();

        world->m_seed = wi.seed;
        world->m_spawn_position = wi.spawn_position;
    }

    world->m_name = name;

    world->m_dims[overworld].m_world = world.get();
    world->m_dims[underworld].m_world = world.get();

    world->m_mob_spawner = std::make_unique<MobSpawner>(*world, overworld);

    return world;
}

World::~World()
{
}

void World::change_dimension(EntityId id, int new_dimension)
{
    if (new_dimension < 0 || new_dimension >= int(max_dimensions))
        return;
    const auto entity = get_entity(id);
    if (!entity || entity->m_dimension == new_dimension)
        return;

    Dimension& previous = m_dims[entity->m_dimension];
    previous.remove_entity(entity);
    std::erase(previous.m_entities_to_add, entity);
    entity->m_dimension = new_dimension;
    entity->m_velocity = glm::dvec3(0.0);
    entity->m_on_ground = false;
    entity->m_inside_portal = true;
    // Moving an existing entity must not call on_ready() again.
    Dimension& destination = m_dims[new_dimension];
    std::erase(destination.m_entities_to_remove, entity);
    if (std::find(destination.m_entities.begin(), destination.m_entities.end(), entity) == destination.m_entities.end() &&
        std::find(destination.m_entities_to_add.begin(), destination.m_entities_to_add.end(), entity) == destination.m_entities_to_add.end())
        destination.add_entity(entity);
    cancel_portal_travel(id);

    if (!m_proxy && Engine::get().server())
    {
        const RpcCallPacket packet{
            .id = id,
            .name = "sync_dimension",
            .args = {int64_t(new_dimension), entity->get_position()},
        };
        Engine::get().server()->route_packet(NetworkConnection::create_packet(packet));
    }
}

std::vector<BlockPos> World::portal_generation_origins(int dimension) const
{
    std::vector<BlockPos> origins;
    for (const auto& [id, destination] : m_portal_destinations)
        if (destination.first == dimension)
            origins.push_back(destination.second);
    return origins;
}

bool World::travel_through_portal(EntityId id)
{
    const auto entity = get_entity(id);
    if (m_proxy || !entity)
        return false;

    const int target_dimension = entity->get_dimension() == int(overworld) ? int(underworld) : int(overworld);
    const double scale = target_dimension == int(underworld) ? 1.0 / 8.0 : 8.0;
    const glm::dvec3 source = entity->get_position();
    const int64_t x = int64_t(std::round(source.x * scale));
    const int64_t z = int64_t(std::round(source.z * scale));
    m_portal_destinations[id] = {target_dimension, BlockPos(x, 0, z)};
    Dimension& target = m_dims[target_dimension];

    // Keep the player in the source world until the arrival area is loaded.
    for (int64_t cx = chunk_index(x - 16); cx <= chunk_index(x + 16); ++cx)
        for (int64_t cz = chunk_index(z - 16); cz <= chunk_index(z + 16); ++cz)
            if (!target.has_chunk(cx, cz))
                return false;

    std::optional<glm::dvec3> arrival;
    double best_distance = std::numeric_limits<double>::max();
    for (int64_t px = x - 16; px <= x + 16; ++px)
        for (int64_t pz = z - 16; pz <= z + 16; ++pz)
            for (int64_t py = 1; py < Chunk::height - 2; ++py)
            {
                if (target.get_block(px, py, pz).id != Blocks::portal.hash ||
                    target.get_block(px, py + 1, pz).id != Blocks::portal.hash ||
                    !target.has_solid_block(px, py - 1, pz) ||
                    !is_valid_portal(target_dimension, {px, py, pz}))
                    continue;
                const double distance = double((px - x) * (px - x) + (pz - z) * (pz - z));
                if (distance < best_distance)
                {
                    best_distance = distance;
                    arrival = glm::dvec3(px, double(py) + 0.4, pz);
                }
            }

    if (!arrival)
    {
        int64_t floor_y = 70;
        for (int64_t y = Chunk::height - 6; y >= 1; --y)
            if (target.has_solid_block(x, y, z))
            {
                floor_y = y;
                break;
            }

        // Clear a landing pocket and build a floor and a usable return portal.
        for (int64_t px = x - 2; px <= x + 2; ++px)
            for (int64_t pz = z - 2; pz <= z + 2; ++pz)
            {
                target.set_block(px, floor_y, pz, BlockState(Blocks::bedrock));
                for (int64_t y = floor_y + 1; y <= floor_y + 4; ++y)
                {
                    target.set_block(px, y, pz, BlockState());
                    target.remove_tag({px, y, pz}, "water");
                }
            }
        for (int64_t px = x - 1; px <= x + 2; ++px)
            for (int64_t y = floor_y; y <= floor_y + 4; ++y)
                if (px == x - 1 || px == x + 2 || y == floor_y || y == floor_y + 4)
                    target.set_block(px, y, z, BlockState(Blocks::bedrock));
                else
                    target.set_block(px, y, z, BlockState(Blocks::portal));

        arrival = glm::dvec3(x, double(floor_y) + 1.4, z);
        if (const auto server = std::dynamic_pointer_cast<LocalServer>(Engine::get().server()))
            server->sync_portal_chunks(target_dimension, x, z);
    }

    entity->set_position(*arrival);
    change_dimension(id, target_dimension);
    return true;
}

void World::tick(float delta)
{
    for (int dimension = 0; dimension < int(max_dimensions); ++dimension)
        tick_dimension(delta, dimension);

    m_debug_display.update(delta);
}

void World::tick_dimension(float delta, int dimension)
{
    ZoneScoped;

    for (std::shared_ptr<Entity> entity : m_dims[dimension].get_entities())
    {
        if (entity->get_dimension() != dimension)
            continue;
        if (entity->is_active())
            entity->recurse_tick(delta);
        else
            remove_entity(entity->m_dimension, entity);
    }

    for (std::shared_ptr<Entity> entity : m_dims[dimension].m_entities_to_remove)
    {
        auto it = std::find(m_dims[dimension].m_entities.begin(), m_dims[dimension].m_entities.end(), entity);
        if (it != m_dims[dimension].m_entities.end())
            m_dims[dimension].m_entities.erase(it);
    }
    for (std::shared_ptr<Entity> entity : m_dims[dimension].m_entities_to_add)
        if (entity->get_dimension() == dimension)
            m_dims[dimension].m_entities.push_back(entity);

    m_dims[dimension].m_entities_to_remove.clear();
    m_dims[dimension].m_entities_to_add.clear();

    if (!m_proxy && dimension == int(overworld))
        m_mob_spawner->tick(delta);

    {
        ZoneScopedN("Mesh flush");
        std::vector<ChunkPos> rebuild_completed;
        MeshRebuildResult result;
        while (m_dims[dimension].m_mesh_queue_lockless.try_dequeue(result))
        {
            // A chunk may have been unloaded and another instance loaded at
            // the same position while this mesh was being generated.
            auto chunk_iter = m_dims[dimension].m_chunks.find(result.pos);
            if (chunk_iter == m_dims[dimension].m_chunks.end() || chunk_iter->second != result.chunk)
                continue;
            std::shared_ptr<Chunk> chunk = chunk_iter->second;
            chunk->get_slices()[result.slice_index].opaque_mesh = result.opaque_mesh;
            chunk->get_slices()[result.slice_index].water_mesh = result.water_mesh;
            if (result.updates_shadow_mesh)
                chunk->set_shadow_mesh(result.shadow_mesh);
            if (m_dims[dimension].m_chunks_rebuild_queue.erase(result.pos) > 0)
                rebuild_completed.push_back(result.pos);
        }

        for (const ChunkPos pos : rebuild_completed)
        {
            if (m_dims[dimension].m_chunks_rebuild_pending.erase(pos) > 0)
                m_dims[dimension].queue_rebuild(pos);
        }
    }

    if (!m_proxy)
    {
        for (const auto& [pos, chunk] : m_dims[dimension].get_chunks())
        {
            if (chunk->is_modified())
            {
                EXPECT(save_chunk({}, chunk, dimension));
                chunk->clear_modified();
            }
        }
    }

    if (m_player != nullptr)
    {
        ZoneScopedN("calc visible chunks");

        std::shared_ptr<Camera> camera = m_player->get_camera();

        m_dims[dimension].m_visible_chunks.clear();
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

                if (!camera->frustum().contains(aabb) || (chunk->get_slices()[i].opaque_mesh == nullptr && chunk->get_slices()[i].water_mesh == nullptr))
                    continue;

                m_dims[dimension].m_visible_chunks[chunk->pos()].chunk = chunk;
                m_dims[dimension].m_visible_chunks[chunk->pos()].slice_indices.push_back(i);
            }
        }
    }

    m_dims[dimension].m_sun_visible_chunks.clear();
    // The shadow camera is centered at the player and renders camera-relative
    // chunk positions, so culling must test bounds in that same coordinate space.
    const glm::dvec3 sun_origin = m_player != nullptr
                                      ? m_player->get_camera()->get_global_transform().position()
                                      : glm::dvec3(0.0);
    for (const auto& [key, chunk] : m_dims[dimension].m_chunks)
    {
        ChunkPos pos = chunk->pos();
        AABBf aabb = AABBf(-glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2), glm::vec3(Chunk::width / 2.0, Chunk::height / 2.0, Chunk::width / 2))
                         .translate(glm::dvec3((double)pos.x * Chunk::width + Chunk::width / 2.0, double(Chunk::height) / 2.0, (double)pos.z * Chunk::width + Chunk::width / 2.0) - sun_origin);

        if (!m_dims[dimension].m_sun_frustum.contains(aabb))
            continue;

        // Shadows draw one merged mesh per chunk; slice visibility is unused.
        if (chunk->get_shadow_mesh() != nullptr)
            m_dims[dimension].m_sun_visible_chunks.emplace(pos, RenderableChunk{chunk, {}});
    }
}

BlockState World::get_block_state(int dimension, int64_t x, int64_t y, int64_t z) const
{
    return m_dims[dimension].get_block(x, y, z);
}

bool World::try_create_portal(int dimension, BlockPos anchor)
{
    if (m_proxy)
        return false;
    auto& dim = m_dims[dimension];
    const auto match = PortalPattern::find(anchor.x, anchor.y, anchor.z, Blocks::oak_log.hash,
        Blocks::portal.hash, false, [&](int64_t x, int64_t y, int64_t z) {
            if (y < 0 || y >= Chunk::height || !dim.has_chunk(chunk_index(x), chunk_index(z)) ||
                dim.get_tag({x, y, z}, "water"))
                return UINT32_MAX;
            return dim.get_block(x, y, z).id;
        });
    if (!match)
        return false;
    for (int dy = 1; dy < 4; ++dy)
        for (int dx = 1; dx < 3; ++dx)
            dim.set_block(match->x + (match->along_z ? 0 : dx), match->y + dy,
                          match->z + (match->along_z ? dx : 0), BlockState(Blocks::portal));
    if (const auto server = std::dynamic_pointer_cast<LocalServer>(Engine::get().server()))
        server->sync_portal_chunks(dimension, match->x + 1, match->z + 1);
    return true;
}

bool World::is_valid_portal(int dimension, BlockPos pos) const
{
    const auto& dim = m_dims[dimension];
    for (const auto frame : {Blocks::oak_log, Blocks::bedrock})
        if (PortalPattern::find(pos.x, pos.y, pos.z, frame.hash, Blocks::portal.hash, true,
            [&](int64_t x, int64_t y, int64_t z) {
                if (y < 0 || y >= Chunk::height || !dim.has_chunk(chunk_index(x), chunk_index(z)))
                    return UINT32_MAX;
                return dim.get_block(x, y, z).id;
            }))
            return true;
    return false;
}

void World::set_block_state(int dimension, int64_t x, int64_t y, int64_t z, BlockState state)
{
    // Portal items activate a matching frame instead of placing standalone portal cells.
    if (!m_proxy && state.id == Blocks::portal.hash)
    {
        try_create_portal(dimension, {x, y, z});
        return;
    }
    m_dims[dimension].set_block(x, y, z, state);
    if (state.id == Blocks::oak_log.hash)
        try_create_portal(dimension, {x, y, z});
}

std::optional<std::shared_ptr<Chunk>> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[overworld].get_chunk(x, z);
}

std::optional<std::shared_ptr<Chunk>> World::get_chunk(int64_t x, int64_t z)
{
    return m_dims[overworld].get_chunk(x, z);
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
    bool is_entity = false;
    double closest = range;
    glm::i64vec3 block_pos{};
    glm::dvec3 normal{};
    std::shared_ptr<Entity> entity;

    for (const auto& candidate : m_dims[dimension].get_entities())
    {
        if (candidate.get() == ignore || candidate->get_parent() == ignore)
            continue;
        const AABB world_aabb = candidate->get_aabb().translate(candidate->get_global_transform().position());
        double distance = 0.0;
        glm::dvec3 entity_normal{};
        if (ray_intersect_aabb(ray, world_aabb, distance, entity_normal) &&
            distance >= 0.0 && distance <= closest)
        {
            closest = distance;
            hit = true;
            is_entity = true;
            entity = candidate;
        }
    }

    const glm::dvec3 origin = ray.origin();
    const glm::dvec3 direction = ray.dir();
    glm::i64vec3 cell;
    int step[3]{};
    glm::dvec3 initial_normal{};
    double next_boundary[3]{};
    double delta[3]{};
    for (int axis = 0; axis < 3; ++axis)
    {
        const double coordinate = origin[axis] + 0.5;
        const double lower = std::floor(coordinate);
        const bool on_boundary = std::abs(coordinate - lower) <= 1e-12;
        cell[axis] = int64_t(lower);
        step[axis] = direction[axis] > 0.0 ? 1 : direction[axis] < 0.0 ? -1 : 0;
        if (on_boundary && step[axis] < 0)
            --cell[axis];
        if (on_boundary && step[axis] != 0)
            initial_normal[axis] = -double(step[axis]);
        if (step[axis] == 0)
        {
            next_boundary[axis] = std::numeric_limits<double>::infinity();
            delta[axis] = next_boundary[axis];
        }
        else
        {
            const double boundary = double(cell[axis]) + (step[axis] > 0 ? 0.5 : -0.5);
            next_boundary[axis] = (boundary - origin[axis]) / direction[axis];
            delta[axis] = std::abs(1.0 / direction[axis]);
        }
    }

    double entry_distance = 0.0;
    glm::dvec3 entry_normal = initial_normal;
    while (entry_distance <= range && entry_distance <= closest)
    {
        if (!get_block_state(dimension, cell.x, cell.y, cell.z).is_air())
        {
            closest = entry_distance;
            hit = true;
            is_entity = false;
            block_pos = cell;
            normal = entry_normal;
            break;
        }

        int axis = 0;
        if (next_boundary[1] < next_boundary[axis]) axis = 1;
        if (next_boundary[2] < next_boundary[axis]) axis = 2;
        entry_distance = next_boundary[axis];
        if (!std::isfinite(entry_distance))
            break;
        cell[axis] += step[axis];
        next_boundary[axis] += delta[axis];
        entry_normal = glm::dvec3(0.0);
        entry_normal[axis] = -double(step[axis]);
    }

    if (!hit)
        return false;
    result.hit_entity = is_entity;
    result.pos = ray.at(closest);
    result.distance = closest;
    result.block_pos = block_pos;
    result.normal = normal;
    result.entity = entity;
    return true;
}

void World::break_block(int dimension, int64_t x, int64_t y, int64_t z)
{
    BlockState state = get_block_state(dimension, x, y, z);
    set_block_state(dimension, x, y, z, BlockState());

    // Block breaking is predicted on clients via the hit RPC. Drops, however,
    // must only be created by the authoritative server or they cannot be picked up.
    if (Engine::get().is_client())
        return;

    std::optional<Id<Item>> item_opt = Engine::get().registry().to_item(state.id);
    if (!item_opt.has_value())
        return;

    std::shared_ptr<ItemEntity> item_entity = std::make_shared<ItemEntity>(item_opt.value());
    item_entity->set_position(glm::vec3(x, y, z) + glm::vec3(rand_float(-0.6, 0.6), 0, rand_float(-0.6, 0.6)));

    add_entity(dimension, item_entity);

    const AddItemEntityPacket packet{
        .position = item_entity->get_position(),
        .id = item_entity->id(),
        .item_id = item_opt->hash,
        .dimension = dimension,
    };
    Engine::get().server()->route_packet(NetworkConnection::create_packet(packet));
}

std::expected<void, Error> World::save_chunk(std::stop_token token, std::shared_ptr<Chunk> chunk, int dimension)
{
    (void)token;

    if (Engine::get().is_save_disabled())
        return std::expected<void, Error>();

    std::string path = std::format("{}/saves/{}/DIM{}/{}${}/", Filesystem::get_data_directory(), m_name, dimension, chunk->x(), chunk->z());
    TRY(Filesystem::make_dirs(path));

    path.append("blocks.dat");
    TRY_ASSIGN(File file, Filesystem::open_file(path, true));

    std::vector<uint8_t> compressed_data;
    TRY(ZLib::deflate(std::as_bytes(std::span((uint8_t *)chunk->get_blocks(), sizeof(BlockState) * Chunk::block_count)), compressed_data));

    TRY(file.writer().write_raw(compressed_data.data(), compressed_data.size()));

    file.close();

    path = std::format("{}/saves/{}/DIM{}/{}${}/tags.dat", Filesystem::get_data_directory(), m_name, dimension, chunk->x(), chunk->z());
    TRY_ASSIGN(file, Filesystem::open_file(path, true));

    BufferWriter writer;
    Dimension::write_tags(writer, chunk);

    std::vector<uint8_t> tags_data;
    TRY(ZLib::deflate(std::as_bytes(writer.buffer()), tags_data));

    TRY(file.writer().write_raw(tags_data.data(), tags_data.size()));

    file.close();

    return std::expected<void, Error>();
}

std::expected<void, Error> World::save_entity(const std::shared_ptr<Entity>& entity)
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

    return std::expected<void, Error>();
}

std::expected<void, Error> World::save_player(const std::shared_ptr<Player>& player)
{
    std::string path = std::format("{}saves/{}/players/", Filesystem::get_data_directory(), m_name);
    TRY(Filesystem::make_dirs(path));

    path.append(std::format("{}.dat", player->get_username()));

    EntitySerializer serializer;
    serializer.set("dimension", int64_t(player->get_dimension()));
    serializer.set("position", player->get_position());
    serializer.set("rotation", player->get_rotation());
    TRY(player->save(serializer));

    TRY(serializer.save(path));

    return std::expected<void, Error>();
}

bool World::load_player(std::string_view username, std::shared_ptr<Player>& player)
{
    std::string path = std::format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), get_name(), username);

    if (!std::filesystem::exists(path))
        return false;

    EntitySerializer serializer;
    std::expected<void, Error> result = serializer.load(path);

    if (!result.has_value())
    {
        error("player data is corrupted");
        // TODO: maybe copy corrupted data to a `.backup` file before writing over the data.
        return false;
    }

    glm::dvec3 position = serializer.get<glm::dvec3>("position").value_or(get_spawn_position());
    glm::dquat rotation = serializer.get<glm::dquat>("rotation").value_or(glm::identity<glm::dquat>());

    const int64_t dimension = serializer.get<int64_t>("dimension").value_or(0);
    player->m_dimension = dimension >= 0 && dimension < int64_t(max_dimensions) ? int(dimension) : int(overworld);
    player->set_position(position);
    player->set_rotation(rotation);

    result = player->load(serializer);
    if (!result.has_value())
    {
        error("player data is corrupted");
        return false;
    }

    return true;
}

bool World::is_player_saved(std::string_view name) const
{
    std::string path = std::format("{}saves/{}/players/{}.dat", Filesystem::get_data_directory(), m_name, name);
    return std::filesystem::exists(path);
}
