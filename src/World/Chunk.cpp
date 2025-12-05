#include "World/Chunk.hpp"

Chunk::Chunk(int64_t x, int64_t z)
    : m_x(x), m_z(z)
{
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
