#pragma once

#include "Block/Block.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Ref.hpp"
#include "Entity/Entity.hpp"
#include "Item/Item.hpp"
#include "Render/Renderer.hpp"

#include <stb_image.h>

struct Image
{
    const stbi_uc *data;
    int w;
    int h;
    int channels;
    String path;
};

class EntityRegistry
{
public:
    using Constructor = Ref<Entity> (*)();

    struct Entry
    {
        Constructor c;
    };

    template <typename T>
    void register_entity()
    {
        m_entries.put(T::get_static_hash_code(), Entry{.c = []() -> Ref<Entity>
                                                       { return newref<T>().template cast_to<Entity>(); }});
    }

    Result<Ref<Entity>> create_entity(ClassHashCode class_hash);

private:
    HashMap<ClassHashCode, Entry> m_entries;
};

namespace Blocks
{
constexpr Id<Block> stone(1);
constexpr Id<Block> dirt(2);
constexpr Id<Block> crafting_table(3);
} // namespace Blocks

namespace Items
{
constexpr Id<Item> stone_block(1);
constexpr Id<Item> dirt_block(2);
constexpr Id<Item> crafting_table_block(3);
constexpr Id<Item> water_bucket(4);
}; // namespace Items

namespace Entities
{
constexpr Id<Entity> player(1);
constexpr Id<Entity> cow(2);
}; // namespace Entities

class GameRegistry
{
public:
    void register_all();
    Result<void> post_register();

    void add_block(Id<Block> id, Ref<Block> block);
    void add_item(Id<Item> id, Ref<Item> item);

    Ref<Block> get_block(Id<Block> key) const { return m_blocks.get(key).value_or(nullptr); }
    Ref<Item> get_item(Id<Item> key) const { return m_items.get(key).value_or(nullptr); }

    Option<Id<Block>> to_block(Id<Item> id);
    Option<Id<Item>> to_item(Id<Block> block) { return m_block_items.get(block); }

    Ref<Block> block_from_item(Id<Item> key)
    {
        Ref<Item> item = m_items.get(key).value_or(nullptr);
        if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
            return m_blocks.get(ib->block()).value_or(nullptr);
        return nullptr;
    }

    Ref<Texture> get_texture(Id<Item> item);

    Ref<Texture> get_texture_array() const { return m_texture_array; }

    size_t load_texture(const StringView& path);

    /**
     * Create a texture from a file path. If an error occurs the missing texture is returned.
     */
    Ref<Texture> create_texture(const StringView& path);

    /**
     * Render a 3D preview of the block. If an error occurs the missing texture is returned.
     */
    Ref<Texture> create_preview_texture(Ref<Block> block);

    void register_rpc(ClassHashCode cls, String name, RpcTarget target)
    {
        m_exposed_rpc.get_or_put(cls, {})->put(name, target);
    }

    Option<RpcTarget> get_rpc(Entity *entity, const StringView& name) const
    {
        for (ssize_t i = (ssize_t)entity->get_classes().size(); i >= 0; i--)
        {
            ClassHashCode class_hash = entity->get_classes()[i];
            if (m_exposed_rpc.contains(class_hash))
            {
                const auto rpcs = m_exposed_rpc.get(class_hash).value();
                if (rpcs.contains(name))
                    return rpcs.get(name);
            }
        }
        return None;
    }

private:
    Map<Id<Block>, Ref<Block>> m_blocks;
    Map<Id<Item>, Ref<Item>> m_items;

    Map<Id<Block>, Id<Item>> m_block_items;

    Map<uint16_t, Id<Block>> m_block_ids;

    LocalVector<Image> m_images;
    Ref<Texture> m_texture_array;
    LocalVector<Ref<Texture>> m_texture_handles;

    Map<ClassHashCode, HashMap<String, RpcTarget>> m_exposed_rpc;

    Option<size_t> get_image(const StringView& path);
};
