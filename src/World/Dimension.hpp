#pragma once

#include "AABB.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Containers/LocalVector.hpp"
#include "Entity/Entity.hpp"
#include "World/Chunk.hpp"
#include "World/Generator.hpp"

#include <mutex>
#include <set>

class Dimension
{
    friend class World;

public:
    Dimension()
    {
    }

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const;

    bool has_chunk(int64_t x, int64_t z) const;

    Result<void> add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk);
    void remove_chunk(int64_t x, int64_t z);

    Ref<Entity> get_entity(EntityId id) const;
    Result<void> add_entity(Ref<Entity> entity);
    void remove_entity(EntityId id);

    ALWAYS_INLINE const std::map<ChunkPos, Ref<Chunk>>& get_chunks() const { return m_chunks; }

    const LocalVector<Ref<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

    Vector<AABB> get_boxes_that_may_collide(const AABB& box) const;
    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    Result<Ref<Chunk>> generate_chunk(int64_t cx, int64_t cz);
    BlockState generate_block(int64_t x, int64_t y, int64_t z);

private:
    std::mutex m_chunk_mutex;
    LocalVector<Ref<Entity>> m_entities;

    /**
     * Chunks loaded in memory. Could also be called `Simulated chunks`.
     */
    std::map<ChunkPos, Ref<Chunk>> m_chunks;

    std::mutex m_chunk_loading_mutex;
    std::set<ChunkPos> m_chunk_loading_queue;

    std::map<ChunkPos, Ref<Chunk>> m_chunks_to_flush;
    Vector<Ref<GenerationPass>> m_generation_passes;
};
