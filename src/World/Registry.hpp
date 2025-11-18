#pragma once

#include "Core/Filesystem.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "World/Block.hpp"

#include <SDL3_image/SDL_image.h>

#include <bitset>
#include <map>

class BlockRegistry
{
public:
    static void destroy()
    {
        s_texture_array = nullptr;
    }

    static inline bool is_generic(uint16_t id)
    {
        return s_generics.test(id);
    }

    /**
     * @brief Returns the block id for `name` or `0`.
     */
    static uint16_t get_block_id(const std::string& name)
    {
        const auto& el = s_blocks_by_name.find(name);
        if (el == s_blocks_by_name.end())
        {
            [[unlikely]] return 0;
        }
        return el->second;
    }

    static void load_blocks();

    static void register_block(Ref<Block> block)
    {
        const uint16_t id = s_blocks.size() + 1;

        s_blocks.push_back(block);
        s_blocks_by_name[block->name()] = id;

        switch (block->get_variant())
        {
        case BlockStateVariant::Generic:
            s_generics.set(id);
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

    static void create_gpu_resources()
    {
        auto texture_array_result = RenderingDriver::get()->create_texture(16, 16, TextureFormat::RGBA8Srgb, TextureUsageFlagBits::CopyDest | TextureUsageFlagBits::Sampled, TextureDimension::D2DArray, s_textures.size());
        ERR_COND_R(texture_array_result->is_null(), "Cannot create the block texture array");

        s_texture_array = texture_array_result.value();

        size_t index = 0;

        for (const auto& surface : s_textures)
        {
            s_texture_array->update(View((uint8_t *)surface->pixels, surface->w * surface->h * 4), index);
            index++;
        }

        s_texture_registry_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(glm::uvec4), s_blocks.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value();
        std::vector<glm::uvec4> textures;

        for (const Ref<Block>& block : s_blocks)
        {
            std::array<uint32_t, 6> block_textures = block->get_texture_ids();
            textures.push_back(glm::uvec4(
                (block_textures[0] & 0xFF) | ((block_textures[1] & 0xFF) << 16),
                (block_textures[2] & 0xFF) | ((block_textures[3] & 0xFF) << 16),
                (block_textures[4] & 0xFF) | ((block_textures[5] & 0xFF) << 16),
                0));
        }

        s_texture_registry_buffer->update(View(textures).as_bytes());
    }

    static const Ref<Block>& get_block_by_id(uint16_t id)
    {
        return s_blocks[id - 1];
    }

    static inline const Ref<Texture>& get_texture_array()
    {
        return s_texture_array;
    }

    static inline const Ref<Buffer>& get_texture_buffer()
    {
        return s_texture_registry_buffer;
    }

    static inline const std::vector<Ref<Block>>& get_blocks()
    {
        return s_blocks;
    }

    static inline size_t get_block_count()
    {
        return s_blocks.size();
    }

private:
    static uint32_t get_or_create(const std::string& name)
    {
        const auto id_pair = s_texture_by_name.find(name);

        if (id_pair == s_texture_by_name.end())
        {
            std::string path = "assets://textures/" + name;

            Result<File> file = Filesystem::open_file(path);
            ERR_EXPECT_VR(file, 0, "Failed to open {}", path);

            const std::vector<char>& buffer = file->read_to_buffer();
            SDL_IOStream *texture_stream = SDL_IOFromConstMem(buffer.data(), buffer.size());

            SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
            ERR_COND_V(texture_surface == nullptr, "Failed to parse image {}", path);

            // TODO: Check the format of the image and resize it if necessary.

            const uint32_t id = s_textures.size();

            s_textures.push_back(texture_surface);
            s_texture_by_name[name] = id;

            // SDL_DestroySurface(texture_surface);
            SDL_CloseIO(texture_stream);

            return id;
        }
        else
        {
            return id_pair->second;
        }
    }

    static inline std::vector<Ref<Block>> s_blocks;
    static inline std::map<std::string, uint16_t> s_blocks_by_name;

    // Block ids that use the `generic` variant.
    static inline std::bitset<std::numeric_limits<uint16_t>::max()> s_generics;

    static inline std::map<std::string, uint32_t> s_texture_by_name;
    static inline std::vector<SDL_Surface *> s_textures;
    static inline Ref<Texture> s_texture_array;
    static inline Ref<Buffer> s_texture_registry_buffer;
};
