#pragma once

#include "AABB.hpp"
#include "Entity/Entity.hpp"
#include "Frustum.hpp"
#include "World/Chunk.hpp"
#include "World/Gen.hpp"

#include <mutex>
#include <set>

struct RenderableChunk
{
    std::shared_ptr<Chunk> chunk;
    size_t slice_index;
};

class Dimension
{
    friend class World;

public:
    std::optional<std::shared_ptr<Chunk>> get_chunk(int64_t x, int64_t z) const;

    bool has_chunk(int64_t x, int64_t z) const;

    void add_chunk(int64_t x, int64_t z, const std::shared_ptr<Chunk>& chunk);
    void remove_chunk(int64_t x, int64_t z);

    std::shared_ptr<Entity> get_entity(EntityId id) const;
    void add_entity(std::shared_ptr<Entity> entity);
    void remove_entity(std::shared_ptr<Entity> entity);
    void remove_entity(EntityId id);

    const std::map<ChunkPos, std::shared_ptr<Chunk>>& get_chunks() const { return m_chunks; }
    std::span<const RenderableChunk> get_visible_chunks() const { return m_visible_chunks; }
    std::span<const RenderableChunk> get_sun_visible_chunks() const { return m_sun_visible_chunks; }

    void update_sun(glm::mat4 matrix);

    const std::vector<std::shared_ptr<Entity>>& get_entities() const { return m_entities; }

    std::mutex& mutex() { return m_chunk_mutex; }

    std::vector<AABB> get_boxes_that_may_collide(const AABB& box) const;
    std::vector<std::shared_ptr<Entity>> cast_box(const AABB& box) const;

    BlockState get_block(int64_t x, int64_t y, int64_t z) const;
    void set_block(int64_t x, int64_t y, int64_t z, BlockState state);

    void set_tag(glm::i64vec3 pos, std::string_view name, Variant v);
    void remove_tag(glm::i64vec3 pos, std::string_view name);
    std::optional<Variant> get_tag(glm::i64vec3 pos, std::string_view name) const;

    bool has_solid_block(int64_t x, int64_t y, int64_t z) const;

    Result<std::shared_ptr<Chunk>> generate_chunk(int64_t cx, int64_t cz);
    BlockState generate_block(int64_t x, int64_t y, int64_t z, std::shared_ptr<Chunk>& chunk);

    void rebuild(ChunkPos pos, size_t slice_index = 0, size_t slice_count = Chunk::slice_count);
    void queue_rebuild(ChunkPos pos, size_t slice_index = 0, size_t slice_count = Chunk::slice_count);

private:
    std::mutex m_chunk_mutex;
    std::map<ChunkPos, std::shared_ptr<Chunk>> m_chunks;
    std::vector<RenderableChunk> m_visible_chunks;

    Frustum m_sun_frustum;
    std::vector<RenderableChunk> m_sun_visible_chunks;

    std::vector<std::shared_ptr<Entity>> m_entities;
    std::vector<std::shared_ptr<Entity>> m_entities_to_add;
    std::vector<std::shared_ptr<Entity>> m_entities_to_remove;

    std::mutex m_chunk_loading_mutex;
    std::set<ChunkPos> m_chunk_loading_queue;

    std::mutex m_chunk_rebuild_mutex;
    std::set<ChunkPos> m_chunk_rebuild_queue;

    std::map<ChunkPos, std::shared_ptr<Chunk>> m_chunks_to_flush;
    std::vector<ChunkPos> m_chunks_to_remove;

    std::shared_ptr<Gen> m_gen;
};
