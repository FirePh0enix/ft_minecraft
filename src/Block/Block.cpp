#include "Block/Block.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

Block::Block(const Array<String, 6>& textures)
    : m_textures(textures), m_transparent(false)
{
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = EXPECT(Engine::get().registry().load_texture(textures[i]));
}

Block::Block(const StringView& texture)
    : m_transparent(false)
{
    set_texture(texture);
}

void Block::set_texture(const StringView& path)
{
    m_textures = {path, path, path, path, path, path};
    uint32_t id = EXPECT(Engine::get().registry().load_texture(path));
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = id;
}
