#pragma once

#include "Entity/Entity.hpp"
#include "World/Chunk.hpp"

#include <mutex>

class Dimension
{
public:
    Dimension()
    {
    }

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t y, int64_t z) const
    {
        auto iter = m_chunks.find(ChunkPos(x, y, z));
        if (iter == m_chunks.end())
            return std::nullopt;
        return iter->second;
    }

    bool has_chunk(int64_t x, int64_t y, int64_t z) const
    {
        auto iter = m_chunks.find(ChunkPos(x, y, z));
        return iter != m_chunks.end();
    }

    void remove_chunk(int64_t x, int64_t y, int64_t z)
    {
        auto iter = m_chunks.find(ChunkPos(x, y, z));
        if (iter == m_chunks.end())
            m_chunks.erase(iter);
    }

    void add_chunk(int64_t x, int64_t y, int64_t z, const Ref<Chunk>& chunk)
    {
        m_chunks[ChunkPos(x, y, z)] = chunk;

        if (!chunk->is_empty())
            m_visible_chunks.push_back(chunk);
    }

    ALWAYS_INLINE const std::map<ChunkPos, Ref<Chunk>>& get_chunks() const { return m_chunks; }
    ALWAYS_INLINE View<Ref<Chunk>> get_visible_chunks() const { return m_visible_chunks; }

    ALWAYS_INLINE void add_entity(const Ref<Entity>& entity) { m_entities.push_back(entity); }
    const std::vector<Ref<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

private:
    std::mutex m_chunk_mutex;
    std::vector<Ref<Entity>> m_entities;

    std::map<ChunkPos, Ref<Chunk>> m_chunks;
    std::vector<Ref<Chunk>> m_visible_chunks;
};
