#pragma once

#include "Core/Class.hpp"
#include "Render/Driver.hpp"
#include "World/Block.hpp"
#include "World/Chunk.hpp"

#include <mutex>
#include <semaphore>
#include <set>
#include <thread>
#include <vector>

class World;

class GeneratorPass : public Object
{
    CLASS(GeneratorPass, Object);

public:
    GeneratorPass() {}

    void set_seed(uint64_t seed)
    {
        m_seed = seed;
        seed_updated();
    }

    virtual void process(int64_t x, int64_t y, int64_t z, int64_t cx, int64_t cz, BlockState *blocks) const = 0;
    virtual void seed_updated() {};

protected:
    uint64_t m_seed = 0;
};

struct ChunkBuffers
{
    std::atomic_bool used = false;
    Ref<Buffer> block_buffer;
    Ref<Buffer> visibility_buffer;
    Ref<Material> material;
};

class Generator : public Object
{
    CLASS(Generator, Object);

public:
    Generator(World *world, size_t dimension);
    ~Generator();

    void add_pass(Ref<GeneratorPass> pass);

    void request_load(int64_t x, int64_t y, int64_t z);
    void request_multiple_load(View<ChunkPos> chunks);
    void request_unload(int64_t x, int64_t y, int64_t z);

    void set_distance(uint32_t distance);
    void load_around(int64_t x, int64_t y, int64_t z);

    Ref<Chunk> generate_chunk(int64_t x, int64_t y, int64_t z);

    void set_reference_pos(const glm::vec3& pos) { m_reference_position = pos; }

private:
    World *m_world;
    size_t m_dimension;

    std::vector<Ref<GeneratorPass>> m_passes;

    std::thread m_load_thread;
    std::mutex m_load_orders_lock;
    std::atomic_bool m_load_state = true;
    std::binary_semaphore m_load_orders_semaphore{0};
    std::set<ChunkPos> m_load_orders;

    ChunkPos pop_nearest_chunk(glm::vec3 position);

    std::thread m_unload_thread;
    std::mutex m_unload_orders_lock;
    std::atomic_bool m_unload_orders_state = true;
    std::binary_semaphore m_unload_orders_semaphore{0};
    std::vector<ChunkPos> m_unload_orders;

    glm::vec3 m_reference_position = glm::vec3();

    int64_t m_load_distance = 1;
    size_t m_max_chunk_count = 1;

    static void load_thread(Generator *g);
    static void unload_thread(Generator *g);
};
