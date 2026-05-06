#pragma once

#include "AABB.hpp"
#include "Core/Print.hpp"
#include "Entity/Entity.hpp"
#include "World/Chunk.hpp"
#include "World/Generator.hpp"

#include <algorithm>
#include <cstddef>
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

    bool has_chunk(int64_t x, int64_t z) const
    {
        return m_chunks.contains(ChunkPos(x, z));
    }

    void remove_chunk(int64_t x, int64_t z)
    {
        auto iter = m_chunks.find(ChunkPos(x, z));
        if (iter != m_chunks.end())
            m_chunks.erase(iter);
    }

    void remove_entity(EntityId id)
    {
        if (id.is_valid())
        {
            m_entities.remove_if(
                [&](const Ref<Entity>& entity)
                {
                    return entity->id() == id;
                });
        }
    }

    Result<void> add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
    {
        m_chunks[ChunkPos(x, z)] = chunk;
        return Result<void>();
    }

    ALWAYS_INLINE const std::map<ChunkPos, Ref<Chunk>>& get_chunks() const { return m_chunks; }

    ALWAYS_INLINE Result<void> add_entity(Ref<Entity> entity) { return m_entities.append(entity); }
    const Vector<Ref<Entity>>& get_entities() const { return m_entities; }

    Ref<Entity> get_entity(EntityId id) const
    {
        for (const auto& entity : m_entities)
        {
            if (entity->id() == id)
                return entity;
        }
        return nullptr;
    }

    std::mutex& mutex() { return m_chunk_mutex; }

    Vector<AABB> get_boxes_that_may_collide(const AABB& box) const;
    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    Result<Ref<Chunk>> generate_chunk(int64_t cx, int64_t cz);

private:
    std::mutex m_chunk_mutex;
    Vector<Ref<Entity>> m_entities;
    World *m_world = nullptr;

    /**
     * Chunks loaded in memory. Could also be called `Simulated chunks`.
     */
    std::map<ChunkPos, Ref<Chunk>> m_chunks;
    /**
     * Chunks rendered to the screen.
     */
    std::map<ChunkPos, Ref<Chunk>> m_render_chunks;

    std::mutex m_chunk_loading_mutex;
    std::set<ChunkPos> m_chunk_loading_queue;

    Vector<Ref<Chunk>> m_chunks_to_flush;
    // TODO: add a `std::set` for checking if chunks exists.

    Vector<Ref<GenerationPass>> m_generation_passes;
};
