#include "World/Block.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

Block::Block(const Array<String, 6>& textures)
    : m_textures(textures), m_transparent(false)
{
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = EXPECT(Engine::get().registry().load_texture(textures[i]));
}

Block::Block(const StringView& texture)
    : m_textures({texture, texture, texture, texture, texture, texture}), m_transparent(false)
{
    uint32_t id = EXPECT(Engine::get().registry().load_texture(texture));
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = id;
}
