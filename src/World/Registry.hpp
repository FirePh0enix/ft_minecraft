#pragma once

#include "Core/Ref.hpp"
#include "Core/String.hpp"
#include "Entity/Entity.hpp"
#include "Render/Renderer.hpp"
#include "World/Block.hpp"

#include <SDL3_image/SDL_image.h>

#include <functional>
#include <map>

class BlockRegistry
{
public:
    BlockRegistry();
    ~BlockRegistry();

    Result<void> register_block(StringView name);

    /**
     * @brief Returns the block id for `name` or `0`.
     */
    uint16_t get_block_id(const StringView& name)
    {
        const auto& el = m_blocks_by_name.find(name);
        if (el == m_blocks_by_name.end())
        {
            [[unlikely]] return 0;
        }
        return el->second;
    }

    const Ref<Block>& get_block_by_id(uint16_t id)
    {
        return m_blocks.get_unchecked(id - 1);
    }

    const Ref<Block>& get_block_by_name(const StringView& name)
    {
        return m_blocks.get_unchecked(get_block_id(name) - 1);
    }

    void create_gpu_resources();

    inline const Ref<Texture>& get_texture_array()
    {
        return m_texture_array;
    }

    inline const Ref<Texture>& get_texture(size_t index)
    {
        return m_texture_handles.get_unchecked(index);
    }

private:
    LocalVector<Ref<Block>> m_blocks;
    std::map<String, uint16_t> m_blocks_by_name;
    std::map<String, uint32_t> m_texture_by_name;
    Vector<SDL_Surface *> m_textures;
    LocalVector<Ref<Texture>> m_texture_handles;
    Ref<Texture> m_texture_array;
    Ref<Buffer> m_texture_registry_buffer;

    uint32_t get_or_create(const StringView& name);
};

class EntityRegistry
{
public:
    using Constructor = std::function<Result<Ref<Entity>>()>;

    struct Entry
    {
        std::optional<Constructor> c;
    };

    template <typename T>
    void register_entity()
    {
        m_entries[T::get_static_hash_code()] = Entry{.c = []() -> Result<Ref<Entity>>
                                                     { return TRY(newref<T>()).template cast_to<Entity>(); }};
    }

    Result<Ref<Entity>> create_entity(ClassHashCode class_hash);

private:
    std::map<ClassHashCode, Entry> m_entries;
};
