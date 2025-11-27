#include "World/Chunk.hpp"
#include "World/World.hpp"

Chunk::Chunk(int64_t x, int64_t z, const Ref<Shader>& surface_shader, const Ref<Shader>& visual_shader, World *world)
    : m_x(x), m_z(z)
{
    m_blocks.resize(block_count);

    m_gpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value();
    m_cpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::CopyDest, BufferVisibility::GPUAndCPU).value();

    m_surface_material = ComputeMaterial::create(surface_shader);
    m_surface_material->set_param("blocks", m_gpu_blocks);
    m_surface_material->set_param("simplexState", world->get_perm_buffer());

    m_visual_material = Material::create(visual_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);
    m_visual_material->set_param("blocks", m_gpu_blocks);
}

void Chunk::generate()
{
    ChunkGPUInfo info((int32_t)m_x, (int32_t)m_z);

    DataBuffer buffer(sizeof(ChunkGPUInfo));
    buffer.add(info);

    ComputePassEncoder encoder = RenderGraph::get().compute_pass_begin();
    encoder.bind_material(m_surface_material);
    encoder.push_constants(buffer);
    encoder.dispatch(16, 1, 16);
    encoder.end();
    // RenderGraph::get().copy_buffer(m_cpu_blocks, m_gpu_blocks);

    // m_cpu_blocks->read_async([](const void *data, size_t size, void *user)
    //                          { Chunk *chunk = (Chunk *)user; std::memcpy(chunk->m_blocks.data(), data, size); }, this);
}
