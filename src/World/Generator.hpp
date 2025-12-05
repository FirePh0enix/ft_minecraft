#pragma once

#include "World/World.hpp"

#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

class GeneratorPass : public Object
{
    CLASS(GeneratorPass, Object);

public:
    GeneratorPass(uint64_t seed) : m_seed(seed) {}

    virtual BlockState process(const std::vector<BlockState>& previous_blocks, int64_t x, int64_t y, int64_t z) const = 0;

protected:
    uint64_t m_seed;
};

struct ChunkBuffers
{
    bool used = false;
    Ref<Buffer> block_buffer;
    Ref<Buffer> visibility_buffer;
    Ref<Material> material;
};

class Generator : public Object
{
    CLASS(Generator, Object);

public:
    Generator(const Ref<World>& world, const Ref<Shader>& shader);

    void request_load(int64_t x, int64_t z);
    void request_unload(int64_t x, int64_t z);

    void load_around(int64_t x, int64_t y, int64_t z);

    Ref<Chunk> generate_chunk(int64_t x, int64_t z);

private:
    Ref<World> m_world;
    std::mutex m_world_mutex;

    std::vector<Ref<GeneratorPass>> m_passes;

    std::thread m_load_thread;
    std::mutex m_load_orders_lock;
    std::atomic_bool m_load_orders_state = true;
    std::binary_semaphore m_load_orders_semaphore{0};
    std::vector<ChunkPos> m_load_orders;
    std::vector<ChunkPos> m_load_orders_processing;

    std::thread m_unload_thread;
    std::mutex m_unload_orders_lock;
    std::atomic_bool m_unload_orders_state = true;
    std::binary_semaphore m_unload_orders_semaphore{0};
    std::vector<ChunkPos> m_unload_orders;
    std::vector<ChunkPos> m_unload_orders_processing;

    size_t m_max_chunk_count = 16;
    std::vector<ChunkBuffers> m_chunk_buffers;
    std::mutex m_chunk_buffers_mutex;

    // Some resources
    Ref<Shader> m_visual_shader;

    static void load_thread(Generator *g);

    size_t acquire_buffers();
};
