#include "World/Registry.hpp"

#include "Block/Block.hpp"
#include "Block/CraftingTable.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Ref.hpp"

Result<Ref<Entity>> EntityRegistry::create_entity(ClassHashCode class_hash)
{
    return m_entries.get(class_hash).get().c();
}

#define TEX(name) ("assets/textures/" name ".png")

Result<void> GameRegistry::register_all()
{
    TRY(add_block(Blocks::stone, TRY(newref<Block>(TEX("stone")))));
    TRY(add_block(Blocks::dirt, TRY(newref<Block>(TEX("dirt")))));
    TRY(add_block(Blocks::water, TRY(newref<Block>(TEX("water")))));
    TRY(add_block(Blocks::crafting_table, TRY(newref<CraftingTableBlock>())));

    TRY(add_item(Items::stone_block, TRY(newref<ItemBlock>(Blocks::stone))));
    TRY(add_item(Items::dirt_block, TRY(newref<ItemBlock>(Blocks::dirt))));
    TRY(add_item(Items::crafting_table_block, TRY(newref<ItemBlock>(Blocks::crafting_table))));
    return Result<void>();
}

Result<void> GameRegistry::post_register()
{
    uint32_t mip_level = 1;
    m_texture_array = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2DArray, m_images.size(), mip_level));

    size_t index = 0;
    for (const auto& image : m_images)
    {
        m_texture_array->update(View((uint8_t *)image.data, image.w * image.h * 4), index);

        Ref<Texture> texture = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2D));
        texture->update(View((uint8_t *)image.data, image.w * image.h * 4));

        // TODO: create textureview instead of duplicating data in memory.
        EXPECT(m_texture_handles.append(texture));
        index++;
    }

    // s_texture_array->generate_mips();
    return Result<void>();
}

Result<void> GameRegistry::add_block(Id<Block> id, Ref<Block> block)
{
    TRY(m_blocks.put(id, block));
    return Result<void>();
}

Result<void> GameRegistry::add_item(Id<Item> id, Ref<Item> item)
{
    TRY(m_items.put(id, item));

    if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
        TRY(m_block_items.put(ib->block(), id));

    return Result<void>();
}

Option<Id<Block>> GameRegistry::to_block(Id<Item> id)
{
    if (!id.valid())
        return None;

    Option<Ref<Item>> item_opt = m_items.get(id);
    if (!item_opt.has_value())
        return None;

    if (Ref<ItemBlock> ib = item_opt.get().cast_to<ItemBlock>())
    {
        return ib->block();
    }
    return None;
}

Ref<Texture> GameRegistry::get_texture(Id<Item> id)
{
    ASSERT_V(id.valid() && id.value <= m_items.size(), "Trying to get texture for invalid id `{}`", id.value);

    Option<Ref<Item>> item = m_items.get(id);
    if (item.has_value())
    {
        Ref<Item> i = item.get();
        if (Ref<ItemBlock> ib = i.cast_to<ItemBlock>())
        {
            Id<Block> block_id = ib->block();
            Ref<Block> b = m_blocks.get(block_id).get();
            return m_texture_handles.get_unchecked(b->get_texture_ids()[0]);
        }
    }
    return nullptr;
}

Result<size_t> GameRegistry::load_texture(const StringView& path)
{
    Option<size_t> i = get_image(path);
    if (i.has_value())
        return i.get();

    File file = EXPECT(Filesystem::open_file(path));
    LocalVector<char> buffer;
    EXPECT(file.reader().read_to_buffer(buffer));
    file.close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);

    const size_t id = m_images.size();
    TRY(m_images.append(Image(data, w, h, channels, path)));
    return id;
}

Option<size_t> GameRegistry::get_image(const StringView& path)
{
    for (size_t i = 0; i < m_images.size(); i++)
    {
        if (StringView(m_images.get_unchecked(i).path) == path)
            return i;
    }
    return None;
}
