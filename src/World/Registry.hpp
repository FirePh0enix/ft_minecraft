#pragma once

#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "World/Block.hpp"

#include <SDL3_image/SDL_image.h>

#include <bitset>
#include <map>
#include <print>

class BlockRegistry
{
public:
    static BlockRegistry& get()
    {
        return singleton;
    }

    void destroy()
    {
        m_texture_array = nullptr;
    }

    inline bool is_generic(uint16_t id)
    {
        return m_generics.test(id);
    }

    /**
     * @brief Returns the block id for `name` or `0`.
     */
    uint16_t get_block_id(const std::string& name) const
    {
        const auto& el = m_blocks_by_name.find(name);
        if (el == m_blocks_by_name.end())
        {
            [[unlikely]] return el->second;
        }
        return 0;
    }

    void register_block(Ref<Block> block)
    {
        const uint16_t id = m_blocks.size() + 1;

        m_blocks.push_back(block);
        m_blocks_by_name[block->name()] = id;

        switch (block->get_variant())
        {
        case BlockStateVariant::Generic:
            m_generics.set(id);
            break;
        }

        block->set_texture_ids({
            get_or_create(block->get_texture_names()[0]),
            get_or_create(block->get_texture_names()[1]),
            get_or_create(block->get_texture_names()[2]),
            get_or_create(block->get_texture_names()[3]),
            get_or_create(block->get_texture_names()[4]),
            get_or_create(block->get_texture_names()[5]),
        });
    }

    void create_texture_array()
    {
        auto texture_array_result = RenderingDriver::get()->create_texture_array(16, 16, TextureFormat::RGBA8Srgb, {.copy_dst = true, .sampled = true}, m_textures.size());
        ERR_COND_R(texture_array_result->is_null(), "Cannot create the block texture array");

        m_texture_array = texture_array_result.value();
        m_texture_array->transition_layout(TextureLayout::CopyDst);

        size_t index = 0;

        for (const auto& surface : m_textures)
        {
            m_texture_array->update(Span((uint8_t *)surface->pixels, surface->w * surface->h * 4), index);
            index++;
        }

        m_texture_array->transition_layout(TextureLayout::ShaderReadOnly);
    }

    const Ref<Block>& get_block_by_id(uint16_t id)
    {
        return m_blocks[id];
    }

    inline const Ref<Texture>& get_texture_array() const
    {
        return m_texture_array;
    }

    inline const std::vector<Ref<Block>>& get_blocks()
    {
        return m_blocks;
    }

    inline size_t get_block_count() const
    {
        return m_blocks.size();
    }

private:
    uint32_t get_or_create(const std::string& name)
    {
        const auto id_pair = m_texture_by_name.find(name);

        if (id_pair == m_texture_by_name.end())
        {
            std::string path = "assets/textures/" + name;

            SDL_IOStream *texture_stream = SDL_IOFromFile(path.c_str(), "r");
            SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);

            // TODO: Add checks
            // TODO: Check the format of the image and rezise it if necessary.

            const uint32_t id = m_textures.size();

            m_textures.push_back(texture_surface);
            m_texture_by_name[name] = id;

            // SDL_DestroySurface(texture_surface);
            SDL_CloseIO(texture_stream);

            return id;
        }
        else
        {
            return id_pair->second;
        }
    }

    static BlockRegistry singleton;

    std::vector<Ref<Block>> m_blocks;
    std::map<std::string, uint16_t> m_blocks_by_name;

    // Block ids that use the `generic` variant.
    std::bitset<std::numeric_limits<uint16_t>::max()> m_generics;

    std::map<std::string, uint32_t> m_texture_by_name;
    std::vector<SDL_Surface *> m_textures;
    Ref<Texture> m_texture_array;
};
