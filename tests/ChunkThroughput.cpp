// CPU stage benchmark: real terrain, structures, block models and mesh geometry.
// Deliberately excludes disk saves, GPU upload and rendering.
#include "Engine.hpp"
#include "World/World.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
static void hash_bytes(uint64_t& hash, const void* data, size_t size)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i)
        hash = (hash ^ bytes[i]) * 1099511628211ULL;
}
int main()
{
    auto engine = std::make_unique<Engine>(true);
    engine->registry().register_all();
    uint64_t hash = 1469598103934665603ULL;
    size_t vertices = 0, count = 0;
    double preload_ms = 0, generation_ms = 0, opaque_ms = 0, water_ms = 0;
    for (const auto center : {ChunkPos(0, 0), ChunkPos(-128, 77), ChunkPos(512, -512)})
    {
        Dimension dim(0);
        OverworldGen gen(WorldSettings{.seed = 42});
        std::map<ChunkPos, std::shared_ptr<PreLoadedChunk>> preloads;
        std::map<ChunkPos, std::shared_ptr<Chunk>> chunks;
        auto start = Clock::now();
        for (int x = -5; x <= 5; ++x)
            for (int z = -5; z <= 5; ++z)
            {
                ChunkPos pos(center.x + x, center.z + z);
                auto p = std::make_shared<PreLoadedChunk>();
                p->pos = pos;
                gen.preload(pos.x, pos.z, p);
                gen.structure_pass(pos.x, pos.z, p, dim);
                preloads[pos] = p;
            }
        preload_ms += ms(start);
        for (int x = -4; x <= 4; ++x)
            for (int z = -4; z <= 4; ++z)
            {
                ChunkPos pos(center.x + x, center.z + z);
                start = Clock::now();
                auto c = std::make_shared<Chunk>(&dim, pos.x, pos.z);
                gen.generate_chunk(c, preloads.at(pos), dim);
                c->count_blocks();
                generation_ms += ms(start);
                hash_bytes(hash, c->get_blocks(), sizeof(BlockState) * Chunk::block_count);
                hash_bytes(hash, c->get_biomes().data(), c->get_biomes().size_bytes());
                BufferWriter tags;
                Dimension::write_tags(tags, c);
                hash_bytes(hash, tags.buffer().data(), tags.buffer().size());
                chunks[pos] = c;
                ++count;
            }
        std::map<ChunkPos, Chunk::WaterMask> water;
        start = Clock::now();
        for (const auto& [pos, c] : chunks)
            water.emplace(pos, c->build_water_mask());
        water_ms += ms(start);
        for (const auto& [pos, c] : chunks)
            for (size_t slice = 0; slice < Chunk::slice_count; ++slice)
            {
                start = Clock::now();
                auto mesh = c->build_opaque_geometry(slice, chunks);
                opaque_ms += ms(start);
                vertices += mesh.vertex_count();
                start = Clock::now();
                auto water_mesh = c->build_water_geometry(slice, water);
                water_ms += ms(start);
                vertices += water_mesh.vertex_count();
            }
    }
    std::printf("%zu chunks; preload+structures %.2f ms; generation %.2f ms; opaque %.2f ms; water %.2f ms\n",
                count, preload_ms, generation_ms, opaque_ms, water_ms);
    std::printf("per chunk: generation %.3f ms; opaque %.3f ms; water %.3f ms; hash %016llx; vertices %zu\n",
                generation_ms/count, opaque_ms/count, water_ms/count, (unsigned long long)hash, vertices);
    engine->get_thread_pool().stop();
    engine->get_mesh_thread_pool().stop();
    std::fflush(stdout);
    std::_Exit(0);
}
