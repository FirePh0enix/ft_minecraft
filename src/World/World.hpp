#pragma once

#include "Core/Types.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Network/Packet.hpp"
#include "Ray.hpp"
#include "World/Chunk.hpp"
#include "World/Dimension.hpp"

#include <enet/enet.h>

#include <cstddef>

class Player;

inline int64_t chunk_index(int64_t global)
{
    int64_t c = global / 16;
    if (global < 0 && global % 16 != 0)
        --c;
    return c;
}

inline int64_t local_coords(int64_t g)
{
    int64_t loc = g % 16;
    if (loc < 0)
        loc += 16;
    return loc;
}

enum WorldPresetType : uint32_t
{
    WorldPresetFlat,
    WorldPresetNormal,
};

enum class Face
{
    PosX,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ
};

inline glm::vec3 face_normal(Face face)
{
    const glm::vec3 normals[6]{
        glm::vec3(1, 0, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
    };
    return normals[(size_t)face];
}

struct RaycastResult
{
    glm::vec3 pos;
    glm::vec3 normal;
    float distance;
    /**
     * If true, `entity` is a valid reference to an entity else `block_pos` is valid.
     */
    bool hit_entity;

    glm::i64vec3 block_pos;
    std::shared_ptr<Entity> entity;
};

struct WorldSaveInfo
{
    uint64_t seed;
    WorldPresetType type;
    glm::vec3 spawn_position;
};

struct ChunkLoadRequest
{
    ENetPeer *peer;
    int dimension;
    int64_t x;
    int64_t z;
};

class World
{
    friend class Generator;
    friend class Chunk;

public:
    static constexpr size_t overworld = 0;
    static constexpr size_t underworld = 1;
    static constexpr size_t max_dimensions = 2;

    World();
    ~World();

    static Result<std::shared_ptr<World>> create(std::string name, uint64_t seed, int type);
    static Result<std::shared_ptr<World>> create_proxy(uint64_t seed);
    static Result<std::shared_ptr<World>> load(std::string name);

    void tick(float delta);

    void tick_dimension(float delta, int dimension);

    BlockState get_block_state(int dimension, int64_t x, int64_t y, int64_t z) const;
    void set_block_state(int dimension, int64_t x, int64_t y, int64_t z, BlockState state);

    int64_t get_render_distance() const { return m_load_distance; }

    std::optional<std::shared_ptr<Chunk>> get_chunk(int64_t x, int64_t z) const;
    std::optional<std::shared_ptr<Chunk>> get_chunk(int64_t x, int64_t z);

    std::string_view get_name() const { return m_name; }

    void remove_entity(size_t dim, std::shared_ptr<Entity> entity)
    {
        m_dims[dim].remove_entity(entity);
    }

    void remove_entity(size_t dim, EntityId entity)
    {
        m_dims[dim].remove_entity(entity);
    }

    uint64_t seed() const { return m_seed; }

    const Dimension& get_dimension(size_t index) const
    {
        return m_dims[index];
    }

    Dimension& get_dimension(size_t index)
    {
        return m_dims[index];
    }

    // void set_active_camera(std::shared_ptr<Camera> camera);
    // ALWAYS_INLINE std::shared_ptr<Camera> get_active_camera() const { return m_camera; }

    void set_player(Player *player) { m_player = player; }
    Player *get_player() const { return m_player; }

    void add_entity(int dimension, std::shared_ptr<Entity> entity)
    {
        entity->m_world = this;
        entity->m_dimension = dimension;
        if (!m_proxy && (uint32_t)entity->id() == 0)
            entity->m_id = World::next_id();
        entity->on_ready();
        m_dims[dimension].add_entity(entity);
    }

    std::shared_ptr<Entity> get_entity(EntityId id) const
    {
        return m_dims[0].get_entity(id);
    }

    void change_dimension(EntityId id, int new_dimension)
    {
        std::shared_ptr<Entity> entity = get_entity(id);
        int current_dimension = entity->m_dimension;
        entity->m_dimension = new_dimension;

        remove_entity(current_dimension, id);
        add_entity(new_dimension, entity);
    }

    glm::vec3 get_spawn_position() const { return m_spawn_position; }

    /**
     * Cast a ray through the world and returns the first thing it hit.
     *
     * @param ray Describe the ray direction and origin
     * @param range Size of the ray
     * @return true if the ray hit something
     */
    bool raycast(int dimension, const Ray& ray, float range, RaycastResult& result, const Entity *ignore = nullptr);

    /**
     * Break the block and drop an item corresponding to it.
     */
    void break_block(int dimension, int64_t x, int64_t y, int64_t z);

    /**
     * Save chunk to the disk.
     */
    Result<void> save_chunk(std::shared_ptr<Chunk> chunk);

    Result<void> save_entity(const std::shared_ptr<Entity>& entity);
    Result<void> save_player(const std::shared_ptr<Player>& player);

    /**
     * Load player data from the disk.
     */
    void load_player(std::string_view name, std::shared_ptr<Player>& player);

    void queue_receive_chunk(const ChunkDataPacket& p);

    void send_chunk(ENetPeer *peer, const std::shared_ptr<Chunk>& chunk) const;
    void receive_chunk(const ChunkDataPacket& p);

    bool is_player_saved(std::string_view name) const;

    void request_chunk(ENetPeer *peer, int dimension, int64_t x, int64_t z);

    static EntityId next_id()
    {
        static uint32_t id = 0;
        id++;
        return EntityId(id);
    }

private:
    uint64_t m_seed = 0;
    std::string m_name;

    std::array<Dimension, max_dimensions> m_dims;

    // TODO: needs to be 16
    int64_t m_load_distance = 20;
    // std::vector<ChunkLoadElement> m_load_buffer;

    std::vector<ChunkLoadRequest> m_load_requests;

    bool m_proxy = false;

    Player *m_player;

    glm::vec3 m_spawn_position = glm::vec3();

    void find_safe_spawn();
    void load_around_player(int dimension);
    void request_load_around(int dimension);
};
