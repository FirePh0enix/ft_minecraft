#pragma once

#include "Block/Block.hpp"
#include "Core/Containers/HashMap.hpp"
#include "Core/Ref.hpp"
#include "Entity/Entity.hpp"
#include "Item/Item.hpp"
#include "Render/Renderer.hpp"

#include <stb_image.h>

#include <functional>

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
    using Constructor = std::function<Result<Ref<Entity>>()>;

    struct Entry
    {
        Constructor c;
    };

    template <typename T>
    void register_entity()
    {
        EXPECT(m_entries.put(T::get_static_hash_code(), Entry{.c = []() -> Result<Ref<Entity>>
                                                              { return TRY(newref<T>()).template cast_to<Entity>(); }}));
    }

    Result<Ref<Entity>> create_entity(ClassHashCode class_hash);

private:
    HashMap<ClassHashCode, Entry> m_entries;
};

namespace Blocks
{
constexpr Id<Block> stone(1);
constexpr Id<Block> dirt(2);
constexpr Id<Block> water(3);
constexpr Id<Block> crafting_table(4);
} // namespace Blocks

namespace Items
{
constexpr Id<Item> stone_block(1);
constexpr Id<Item> dirt_block(2);
constexpr Id<Item> crafting_table_block(3);
}; // namespace Items

namespace Entities
{
constexpr Id<Entity> player(1);
constexpr Id<Entity> cow(2);
}; // namespace Entities

class GameRegistry
{
public:
    Result<void> register_all();
    Result<void> post_register();

    Result<void> add_block(Id<Block> id, Ref<Block> block);
    Result<void> add_item(Id<Item> id, Ref<Item> item);

    Ref<Block> get_block(Id<Block> key) const { return m_blocks.get(key).value_or(nullptr); }
    Ref<Item> get_item(Id<Item> key) const { return m_items.get(key).value_or(nullptr); }

    Option<Id<Block>> to_block(Id<Item> id);
    Option<Id<Item>> to_item(Id<Block> block) { return m_block_items.get(block); }

    Ref<Block> block_from_item(Id<Item> key)
    {
        Ref<Item> item = m_items.get(key).value_or(nullptr);
        if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
        {
            return m_blocks.get(ib->block()).value_or(nullptr);
        }
        return nullptr;
    }

    Ref<Texture> get_texture(Id<Item> item);

    Ref<Texture> get_texture_array() const { return m_texture_array; }

    Result<size_t> load_texture(const StringView& path);

private:
    HashMap<Id<Block>, Ref<Block>> m_blocks;
    HashMap<Id<Item>, Ref<Item>> m_items;

    HashMap<Id<Block>, Id<Item>> m_block_items;

    HashMap<uint16_t, Id<Block>> m_block_ids;

    LocalVector<Image> m_images;
    Ref<Texture> m_texture_array;
    LocalVector<Ref<Texture>> m_texture_handles;

    Option<size_t> get_image(const StringView& path);
};
