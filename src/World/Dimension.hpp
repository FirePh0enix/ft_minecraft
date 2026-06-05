#pragma once

#include "AABB.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Core/Containers/Map.hpp"
#include "Core/Containers/Set.hpp"
#include "Entity/Entity.hpp"
#include "World/Chunk.hpp"
#include "World/Generator.hpp"

#include <mutex>

class Dimension
{
    friend class World;

public:
    Dimension()
    {
    }

    Option<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;

    bool has_chunk(int64_t x, int64_t z) const;

    void add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk);
    void remove_chunk(int64_t x, int64_t z);

    Ref<Entity> get_entity(EntityId id) const;
    void add_entity(Ref<Entity> entity);
    void remove_entity(Ref<Entity> entity);

    ALWAYS_INLINE const Map<ChunkPos, Ref<Chunk>>& get_chunks() const { return m_chunks; }

    const LocalVector<Ref<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

    Vector<AABB> get_boxes_that_may_collide(const AABB& box) const;
    Vector<Ref<Entity>> cast_box(const AABB& box) const;

    BlockState get_block(int64_t x, int64_t y, int64_t z) const;
    void set_block(int64_t x, int64_t y, int64_t z, BlockState state);

    void set_tag(glm::i64vec3 pos, const StringView& name, Variant v);
    void remove_tag(glm::i64vec3 pos, const StringView& name);
    Option<Variant> get_tag(glm::i64vec3 pos, const StringView& name) const;

    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    Result<void> rebuild_neighbor_chunks_water(int64_t cx, int64_t cz, size_t slice_index = std::numeric_limits<size_t>::max());

    Result<Ref<Chunk>> generate_chunk(int64_t cx, int64_t cz);
    BlockState generate_block(int64_t x, int64_t y, int64_t z, Ref<Chunk>& chunk);

private:
    std::mutex m_chunk_mutex;
    LocalVector<Ref<Entity>> m_entities;

    LocalVector<Ref<Entity>> m_entities_to_add;
    LocalVector<Ref<Entity>> m_entities_to_remove;

    /**
     * Chunks loaded in memory. Could also be called `Simulated chunks`.
     * NOTE: This map is only should only be accessed from the main thread. For adding chunks from other threads use `m_chunks_to_flush` and `m_chunk_mutex`.
     */
    Map<ChunkPos, Ref<Chunk>> m_chunks;

    std::mutex m_chunk_loading_mutex;
    Set<ChunkPos> m_chunk_loading_queue;

    Map<ChunkPos, Ref<Chunk>> m_chunks_to_flush;
    LocalVector<ChunkPos> m_chunks_to_remove;

    LocalVector<Ref<GenerationPass>> m_generation_passes;
};
