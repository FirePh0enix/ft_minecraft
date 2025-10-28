#include "World/Chunk.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

Chunk::Chunk(int64_t x, int64_t z, const Ref<Shader>& shader)
    : m_x(x), m_z(z)
{
    m_blocks.resize(block_count);
    m_surface_material = ComputeMaterial::create(shader);

    m_gpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::Uniform | BufferUsageFlagBits::Storage).value();
    m_gpu_info = RenderingDriver::get()->create_buffer(STRUCTNAME(ChunkGPUInfo), 1, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Uniform).value();

    ChunkGPUInfo info(x, z);
    m_gpu_info->update(View(info).as_bytes(), 0);

    m_surface_material->set_param("info", m_gpu_info);
    m_surface_material->set_param("blocks", m_gpu_blocks);
}

void Chunk::generate()
{
    ComputePassEncoder encoder = RenderGraph::get().compute_pass_begin();
    encoder.bind_material(m_surface_material);
    encoder.dispatch(1, 256, 1);
}
