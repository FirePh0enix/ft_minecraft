#include "World/Chunk.hpp"
#include "Physics/Collider.hpp"
#include "World/Registry.hpp"

Chunk::Chunk(int64_t x, int64_t z)
    : m_x(x), m_z(z)
{
}

void Chunk::set_blocks(const std::vector<BlockState>& blocks)
{
    m_blocks = blocks;
}

void Chunk::set_buffers(const Ref<Shader>& visual_shader, const Ref<Buffer>& position_buffer)
{
    m_block_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(BlockState), Chunk::block_count, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value_or(nullptr);
    m_visibility_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(uint32_t), Chunk::block_count, BufferUsageFlagBits::CopySource | BufferUsageFlagBits::Storage).value_or(nullptr);

    m_visual_material = Material::create(visual_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back);
    m_visual_material->set_param("blocks", m_block_buffer);
    m_visual_material->set_param("visibilityBuffer", m_visibility_buffer);
    m_visual_material->set_param("positions", position_buffer);
    m_visual_material->set_param("textureRegistry", BlockRegistry::get_texture_buffer());
    m_visual_material->set_param("images", BlockRegistry::get_texture_array());

    // TODO: visibility buffer
    m_block_buffer->update(View(m_blocks).as_bytes());
}

void Chunk::update_grid_collider(GridCollider *grid) const
{
    for (size_t x = 0; x < 16; x++)
        for (size_t y = 0; y < 256; y++)
            for (size_t z = 0; z < 16; z++)
                grid->set(x, y, z);
}
