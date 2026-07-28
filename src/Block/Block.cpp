#include "Block/Block.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

Block::Block(const std::array<std::string, 6>& textures)
    : m_textures(textures), m_transparent(false)
{
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = Engine::get().registry().load_texture(textures[i]);
}

Block::Block(std::string_view texture)
    : m_transparent(false)
{
    set_texture(texture);
}

void Block::set_texture(std::string_view path)
{
    std::string paths;
    m_textures = {paths, paths, paths, paths, paths, paths};
    uint32_t id = Engine::get().registry().load_texture(path);
    for (size_t i = 0; i < 6; i++)
        m_texture_ids[i] = id;
}
