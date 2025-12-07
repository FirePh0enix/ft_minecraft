#pragma once

#include "World/Chunk.hpp"

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

    std::vector<Ref<Entity>>& get_chunks_to_add() { return m_chunks_to_add; }
    std::vector<EntityId>& get_chunks_to_remove() { return m_chunks_to_remove; }

private:
    std::map<ChunkPos, Ref<Chunk>>
        m_chunks;
    std::map<ChunkPos, EntityId> m_collision_chunks;

    std::vector<Ref<Entity>> m_chunks_to_add;
    std::vector<EntityId> m_chunks_to_remove;
};
