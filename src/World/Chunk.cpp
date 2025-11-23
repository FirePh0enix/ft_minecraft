#include "World/Chunk.hpp"
#include "World/World.hpp"

Chunk::Chunk(int64_t x, int64_t z, const Ref<Shader>& shader, World *world)
    : m_x(x), m_z(z)
{
    m_blocks.resize(block_count);
    m_surface_material = ComputeMaterial::create(shader);

    m_gpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::Uniform | BufferUsageFlagBits::Storage | BufferUsageFlagBits::CopySource, BufferVisibility::GPUOnly).value();
    m_cpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::CopyDest, BufferVisibility::GPUAndCPU).value();
    m_gpu_info = RenderingDriver::get()->create_buffer(STRUCTNAME(ChunkGPUInfo), 1, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Uniform).value();

    ChunkGPUInfo info(x, z);
    m_gpu_info->update(View(info).as_bytes(), 0);

    m_surface_material->set_param("info", m_gpu_info);
    m_surface_material->set_param("blocks", m_gpu_blocks);
    m_surface_material->set_param("simplexState", world->get_perm_buffer());
}

void Chunk::generate()
{
    ComputePassEncoder encoder = RenderGraph::get().compute_pass_begin();
    encoder.bind_material(m_surface_material);
    encoder.dispatch(16, 256, 16);
    encoder.end();
    RenderGraph::get().copy_buffer(m_cpu_blocks, m_gpu_blocks);

    m_cpu_blocks->read_async([](const void *data, size_t size, void *user)
                             { Chunk *chunk = (Chunk *)user; std::memcpy(chunk->m_blocks.data(), data, size); }, this);
}
