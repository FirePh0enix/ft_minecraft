#pragma once

#include "AABB.hpp"
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

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const
    {
        auto iter = m_chunks.find(ChunkPos(x, z));
        if (iter == m_chunks.end())
            return std::nullopt;
        return iter->second;
    }

    bool has_chunk(int64_t x, int64_t z)
    {
        return m_chunks.contains(ChunkPos(x, z));
    }

    void remove_chunk(int64_t x, int64_t z)
    {
        auto iter = m_chunks.find(ChunkPos(x, z));
        if (iter != m_chunks.end())
            m_chunks.erase(iter);
    }

    Result<void> add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
    {
        m_chunks[ChunkPos(x, z)] = chunk;
        // TRY(m_visible_chunks.append(chunk));
        return Result<void>();
    }

    ALWAYS_INLINE const std::map<ChunkPos, Ref<Chunk>>& get_chunks() const { return m_chunks; }
    // ALWAYS_INLINE const Vector<Ref<Chunk>>& get_visible_chunks() const { return m_visible_chunks; }

    ALWAYS_INLINE Result<void> add_entity(Ref<Entity> entity) { return m_entities.append(entity); }
    const Vector<Ref<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

    Vector<AABB> get_boxes_that_may_collide(const AABB& box) const;
    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    Result<Ref<Chunk>> generate_chunk(int64_t cx, int64_t cz);

private:
    std::mutex m_chunk_mutex;
    Vector<Ref<Entity>> m_entities;
    World *m_world = nullptr;

    std::map<ChunkPos, Ref<Chunk>> m_chunks;

    std::mutex m_chunk_loading_mutex;
    std::set<ChunkPos> m_chunk_loading_queue;

    Vector<Ref<Chunk>> m_chunks_to_flush;

    // Vector<Ref<Chunk>> m_visible_chunks;
    // TODO: add a `std::set` for checking if chunks exists.

    Vector<Ref<GenerationPass>> m_generation_passes;
};
