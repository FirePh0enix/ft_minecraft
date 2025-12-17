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

    std::optional<Ref<Chunk>> get_chunk(int64_t x, int64_t z) const
    {
        auto iter = m_chunks.find(ChunkPos{.x = x, .z = z});
        if (iter == m_chunks.end())
            return std::nullopt;
        return iter->second;
    }

    bool has_chunk(int64_t x, int64_t z) const
    {
        auto iter = m_chunks.find(ChunkPos{.x = x, .z = z});
        return iter != m_chunks.end();
    }

    void remove_chunk(int64_t x, int64_t z)
    {
        auto iter = m_chunks.find(ChunkPos{.x = x, .z = z});
        if (iter == m_chunks.end())
            m_chunks.erase(iter);
    }

    void add_chunk(int64_t x, int64_t z, const Ref<Chunk>& chunk)
    {
        m_chunks[ChunkPos{.x = x, .z = z}] = chunk;
    }

    size_t get_chunk_count() const
    {
        return m_chunks.size();
    }

    std::map<ChunkPos, Ref<Chunk>>::iterator begin() { return m_chunks.begin(); }
    std::map<ChunkPos, Ref<Chunk>>::iterator end() { return m_chunks.end(); }

    ALWAYS_INLINE void add_entity(const Ref<Entity>& entity) { m_entities.push_back(entity); }
    const std::vector<Ref<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

private:
    std::mutex m_chunk_mutex;
    std::map<ChunkPos, Ref<Chunk>> m_chunks;
    std::vector<Ref<Entity>> m_entities;
};
