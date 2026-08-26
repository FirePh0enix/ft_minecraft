#pragma once

#include "AABB.hpp"
#include "Core/IO.hpp"
#include "Entity/Entity.hpp"
#include "Frustum.hpp"
#include "World/Chunk.hpp"
#include "World/Gen.hpp"

// #include "daking/MPSC_queue.hpp"
#include "MPSCQueue.hpp"

#include <mutex>
#include <set>

class World;
class Dimension;

struct RenderableChunk
{
    Chunk *chunk;
    size_t slice_index;
};

struct ChunkLoadWithDistance
{
    ChunkPos pos;
    float distance = 0.0f;

    bool operator<(const ChunkLoadWithDistance& other) const { return distance < other.distance; }
};

struct PreLoadedChunk
{
    ChunkPos pos;
    Biome *biomes;
    int64_t *heights;

    PreLoadedChunk()
        : biomes(new Biome[16 * 16](Biome::Plain)), heights(new int64_t[16 * 16](0))
    {
    }

    ~PreLoadedChunk()
    {
        delete[] biomes;
        delete[] heights;
    }
};

struct MeshRebuildResult
{
    ChunkPos pos;
    size_t slice_index;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Mesh> water_mesh;
};

/// When generating chunks on multiple threads, we need to guarantee that each pass are generated one after the other, otherwise we will cut structures that overlap
/// multiple chunks.
class GenScheduler
{
public:
    friend class World;

    GenScheduler(Dimension& dimension)
        : m_dimension(dimension)
    {
        // m_chunks.resize(thread_pool_capacity);
    }

    ~GenScheduler()
    {
        // delete[] m_mutexes;
    }

    void terrain_pass(ChunkPos middle);
    void chunk_pass(ChunkPos middle);

    /// Flush chunks that have been generated without waiting for a mutex to unlock.
    void try_flush(std::set<ChunkPos>& chunks);

private:
    Dimension& m_dimension;

    int64_t m_chunk_distance = 16;
    int64_t m_gen_distance = 15;

    std::mutex m_pregen_queue_mutex;
    std::atomic_size_t m_pregen_count = 0;
    std::set<ChunkPos> m_pregen_queue;

    std::vector<ChunkPos> m_pregen_unload_queue;

    void terrain_and_struct_chunk(ChunkPos pos);
    void realize_chunk(ChunkPos pos, std::shared_ptr<PreLoadedChunk> pregen_chunk);
};

class Dimension
{
    friend class World;
    friend class OverworldGen;
    friend class GenScheduler;

public:
    Dimension(int id);

    std::optional<Chunk *> get_chunk(int64_t x, int64_t z) const;

    bool has_chunk(int64_t x, int64_t z) const;
    bool has_pregen_chunk(int64_t x, int64_t z) const;

    std::shared_ptr<Entity> get_entity(EntityId id) const;
    void add_entity(std::shared_ptr<Entity> entity);
    void remove_entity(std::shared_ptr<Entity> entity);
    void remove_entity(EntityId id);

    const std::map<ChunkPos, Chunk *>& get_chunks() const { return m_chunks; }
    std::span<const RenderableChunk> get_visible_chunks() const { return m_visible_chunks; }
    std::span<const RenderableChunk> get_sun_visible_chunks() const { return m_sun_visible_chunks; }

    /// Load and world generation logic.
    void load(int64_t x, int64_t y, int64_t z, int64_t distance);

    /// TODO: remove this, put rendering outside this class.
    void update_sun(glm::mat4 matrix);

    const std::vector<std::shared_ptr<Entity>>& get_entities() const { return m_entities; }

    std::vector<AABBd> get_boxes_that_may_collide(const AABBd& box) const;
    std::vector<std::shared_ptr<Entity>> cast_box(const AABBd& box) const;

    BlockState get_block(int64_t x, int64_t y, int64_t z) const;
    void set_block(int64_t x, int64_t y, int64_t z, BlockState state);

    void set_tag(glm::i64vec3 pos, std::string_view name, std::string v);
    void remove_tag(glm::i64vec3 pos, std::string_view name);
    std::optional<Variant> get_tag(glm::i64vec3 pos, std::string_view name) const;

    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    void rebuild(Chunk *chunk, const std::map<ChunkPos, Chunk *>& nchunks, size_t slice_index, size_t slice_count);
    void queue_rebuild(ChunkPos pos, size_t slice_index = 0, size_t slice_count = Chunk::slice_count);

    void remove_preload(ChunkPos pos);

    void unload_chunk(Chunk *chunk);
    void queue_unload_chunk(Chunk *chunk);

    void place_structure(glm::i64vec3 pos, BlockState *blocks, int64_t w, int64_t h, int64_t l);
    void get_structures_overlap(ChunkPos pos, std::vector<StructureGen>& structures);

private:
    World *m_world = nullptr;
    int m_id;

    Chunk *m_chunk_pool;
    std::atomic_bool *m_chunk_pool_state;

    std::map<ChunkPos, std::shared_ptr<PreLoadedChunk>> m_preloaded_chunks;
    std::map<ChunkPos, Chunk *> m_chunks;

    std::vector<RenderableChunk> m_visible_chunks;

    MPSCQueue<Chunk *> m_chunks_lockless;
    MPSCQueue<std::shared_ptr<PreLoadedChunk>> m_pregen_chunks_lockless;
    MPSCQueue<MeshRebuildResult> m_mesh_queue_lockless;
    MPSCQueue<Chunk *> m_chunks_unload_queue;

    std::set<ChunkPos> m_pregen_loading_queue;
    std::set<ChunkPos> m_chunks_loading_queue;

    GenScheduler m_scheduler;

    // TODO: move this somewhere else.
    Frustum m_sun_frustum;
    std::vector<RenderableChunk> m_sun_visible_chunks;

    std::vector<std::shared_ptr<Entity>> m_entities;
    std::vector<std::shared_ptr<Entity>> m_entities_to_add;
    std::vector<std::shared_ptr<Entity>> m_entities_to_remove;

    std::vector<ChunkLoadWithDistance> m_load_buffer;

    std::mutex m_chunk_rebuild_mutex;
    std::set<ChunkPos> m_chunk_rebuild_queue;

    std::shared_ptr<Gen> m_gen;

    std::mutex m_structures_mutex;
    std::vector<StructureGen> m_structures_queue;

    std::optional<Chunk *> alloc_chunk(int64_t x, int64_t z);
    void free_chunk(Chunk *chunk);

    static void write_tags(Writer& writer, const Chunk *chunk);
    static void read_tags(Reader& reader, Chunk *chunk);
};
