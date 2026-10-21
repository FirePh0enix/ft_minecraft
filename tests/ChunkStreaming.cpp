// Standalone CPU regression fixture; link with the game objects except main.cpp.
#include "Engine.hpp"
#include "World/World.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

static void check(bool condition)
{
    if (!condition)
        std::abort();
}

struct EmptyGen : Gen
{
    EmptyGen() : Gen(WorldSettings{}) {}
    void preload(int64_t, int64_t, std::shared_ptr<PreLoadedChunk>) override {}
    void generate_chunk(std::shared_ptr<Chunk>, std::shared_ptr<PreLoadedChunk>, Dimension&) override {}
};

int main()
{
    // No window/GPU or save writes are required for air-only chunk meshes.
    auto engine = std::make_unique<Engine>(true);
    Dimension dim(0);
    auto marker = std::make_shared<Mesh>(0, WGPUIndexFormat_Uint32, WGPUVertexFormat_Float32x4,
                                        nullptr, nullptr, nullptr, nullptr);
    for (int x = 8; x >= 0; --x)
    {
        auto chunk = std::make_shared<Chunk>(&dim, x, 0);
        chunk->get_slices()[0].opaque_mesh = marker;
        dim.add_chunk(chunk);
        dim.queue_rebuild({x, 0});
        dim.queue_rebuild({x, 0});
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    const auto drain = [&](size_t expected) {
        size_t count = 0;
        while (count < expected)
        {
            count += dim.flush_meshes(1);
            check(std::chrono::steady_clock::now() < deadline);
            std::this_thread::yield();
        }
        check(count == expected);
        check(dim.flush_meshes() == 0);
    };
    dim.dispatch_meshes();
    drain(8 * Chunk::slice_count);
    for (int x = 0; x < 9; ++x)
        check(bool(dim.get_chunk(x, 0).value()->get_slices()[0].opaque_mesh) == (x >= 8));
    std::puts("PASS: nearest chunks first, bounded dispatch, duplicate requests coalesced");

    // An edit while a build is still in flight must survive completion.
    dim.queue_rebuild({0, 0});
    dim.dispatch_meshes();
    dim.queue_rebuild({0, 0});
    drain(2 * Chunk::slice_count);
    dim.dispatch_meshes();
    drain(Chunk::slice_count);
    std::puts("PASS: edits during in-flight rebuilds are retained");

    auto old = dim.get_chunk(0, 0).value();
    dim.queue_rebuild({0, 0});
    dim.dispatch_meshes();
    dim.remove_chunk({0, 0});
    auto replacement = std::make_shared<Chunk>(&dim, 0, 0);
    replacement->get_slices()[0].opaque_mesh = marker;
    dim.add_chunk(replacement);
    drain(Chunk::slice_count);
    check(replacement->get_slices()[0].opaque_mesh == marker);
    dim.queue_rebuild({0, 0});
    dim.dispatch_meshes();
    drain(Chunk::slice_count);
    check(!replacement->get_slices()[0].opaque_mesh);
    std::puts("PASS: late results cannot overwrite replacement chunks or block their meshes");

    AudioMixer audio;
    auto world = World::create("chunk-streaming-cpu-test", 0, 0, audio).value();
    auto& streamed = world->get_dimension(0);
    GenScheduler scheduler(streamed, std::make_shared<EmptyGen>());
    BlockPos origin(0, 0, 0);
    bool predict = false;
    BlockPos predicted(160, 0, 0);
    const auto settle = [&] {
        do
        {
            std::vector<BlockPos> origins{origin};
            if (predict)
                origins.push_back(predicted);
            scheduler.tick(origins);
            streamed.flush_meshes();
            streamed.dispatch_meshes();
            check(std::chrono::steady_clock::now() < deadline);
            std::this_thread::yield();
        } while (scheduler.get_generation_progression() != 1.0f);
    };
    settle();
    const auto radius = ChunkStreaming::load_distance;
    check(streamed.get_chunks().size() == size_t((radius * 2 + 1) * (radius * 2 + 1)));
    check(radius > ChunkStreaming::render_distance);
    // Cross a negative chunk boundary, then turn back: keep the hidden halo.
    origin.x = -17;
    settle();
    check(streamed.has_chunk(-2 - radius, 0));
    origin.x = 0;
    settle();
    check(streamed.has_chunk(radius, 0));
    std::puts("PASS: startup waits for meshes; hidden halo survives movement and reversal");
    predict = true;
    settle();
    check(streamed.has_chunk(radius + 10, 0));
    check(streamed.has_chunk(-radius, 0));
    predicted.x = -160;
    settle();
    check(streamed.has_chunk(-radius - 10, 0));
    check(streamed.has_chunk(radius, 0));
    std::puts("PASS: predicted sprint destination and current neighborhood both become ready, including reversal");
    engine->get_thread_pool().stop();
    engine->get_mesh_thread_pool().stop();
    // Engine teardown expects initialized window/audio; this fixture never initializes them.
    std::fflush(stdout);
    std::_Exit(0);
}
