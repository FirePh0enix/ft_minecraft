#pragma once

#include "Core/Filesystem.hpp"
#include "Core/Ref.hpp"
#include "World/Block.hpp"

#include <SDL3_image/SDL_image.h>

#include <bitset>
#include <map>

class BlockRegistry
{
public:
    static void destroy();

    static inline bool is_generic(uint16_t id)
    {
        return s_generics.test(id);
    }

    /**
     * @brief Returns the block id for `name` or `0`.
     */
    static uint16_t get_block_id(const String& name)
    {
        const auto& el = s_blocks_by_name.find(name);
        if (el == s_blocks_by_name.end())
        {
            [[unlikely]] return 0;
        }
        return el->second;
    }

    static void load_blocks();
    static void register_block(Ref<Block> block);

    static void create_gpu_resources();

    static const Ref<Block>& get_block_by_id(uint16_t id)
    {
        return s_blocks[id - 1];
    }

    // static inline const Ref<Texture>& get_texture_array()
    // {
    //     return s_texture_array;
    // }

    // static inline const Ref<Buffer>& get_texture_buffer()
    // {
    //     return s_texture_registry_buffer;
    // }

    static inline const std::vector<Ref<Block>>& get_blocks()
    {
        return s_blocks;
    }

    static inline size_t get_block_count()
    {
        return s_blocks.size();
    }

private:
    static uint32_t get_or_create(const StringView& name);

    static inline std::vector<Ref<Block>> s_blocks;
    static inline std::map<String, uint16_t> s_blocks_by_name;

    // Block ids that use the `generic` variant.
    static inline std::bitset<std::numeric_limits<uint16_t>::max()> s_generics;

    static inline std::map<String, uint32_t> s_texture_by_name;
    static inline std::vector<SDL_Surface *> s_textures;
    // static inline Ref<Texture> s_texture_array;
    // static inline Ref<Buffer> s_texture_registry_buffer;
};
