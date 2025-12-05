#include "World/Chunk.hpp"
#include "World/World.hpp"

Chunk::Chunk(int64_t x, int64_t z)
    : m_x(x), m_z(z)
{
    // m_gpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value_or(nullptr);
    // m_cpu_blocks = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), block_count, BufferUsageFlagBits::CopyDest, BufferVisibility::GPUAndCPU).value_or(nullptr);
    // m_visibility_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(glm::uvec4), block_count / sizeof(glm::uvec4), BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value();
    // m_visibility_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(uint32_t), block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value_or(nullptr);

    // m_surface_material = ComputeMaterial::create(surface_shader);
    // m_surface_material->set_param("blocks", m_gpu_blocks);
    // m_surface_material->set_param("simplexState", world->get_perm_buffer());

    // m_visibility_material = ComputeMaterial::create(visibility_shader);
    // m_visibility_material->set_param("blocks", m_gpu_blocks);
    // m_visibility_material->set_param("visibilityBuffer", m_visibility_buffer);

    // m_visual_material = Material::create(visual_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);
    // m_visual_material->set_param("blocks", m_gpu_blocks);
    // m_visual_material->set_param("visibilityBuffer", m_visibility_buffer);
}

void Chunk::set_blocks(const std::vector<BlockState>& blocks)
{
    m_blocks = blocks;
}

void Chunk::set_buffers(const Ref<Buffer>& block, const Ref<Buffer>& visibility, const Ref<Material>& material)
{
    m_block_buffer = block;
    m_visibility_buffer = visibility;

    m_visual_material = material;

    // TODO: visibility buffer

    m_block_buffer->update(View(m_blocks).as_bytes());
}

// void Chunk::generate()
// {
//     ChunkGPUInfo info((int32_t)m_x, (int32_t)m_z);

//     DataBuffer buffer(sizeof(ChunkGPUInfo));
//     buffer.add(info);

//     ComputePassEncoder encoder = RenderGraph::get().compute_pass_begin();
//     encoder.bind_material(m_surface_material);
//     encoder.push_constants(buffer);
//     encoder.dispatch(16, 1, 16);
//     encoder.end();

//     ComputePassEncoder encoder2 = RenderGraph::get().compute_pass_begin();
//     encoder2.bind_material(m_visibility_material);
//     encoder2.dispatch(16, 1, 16);
//     encoder2.end();

//     RenderGraph::get().copy_buffer(m_cpu_blocks, m_gpu_blocks);

//     m_cpu_blocks->read_async([](const void *data, size_t size, void *user)
//                              { Chunk *chunk = (Chunk *)user; std::memcpy(chunk->m_blocks.data(), data, size); chunk->ready = true; }, this);
// }
