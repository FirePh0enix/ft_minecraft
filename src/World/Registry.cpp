#include "World/Registry.hpp"

#include "Block/Block.hpp"
#include "Block/CraftingTable.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Ref.hpp"
#include "Engine.hpp"
#include "Item/Bow.hpp"
#include "Item/Bucket.hpp"
#include "Render/Renderer.hpp"
#include "webgpu/webgpu.h"

constexpr int two_d_to_1d(int x, int y, int w)
{
    return y * w + x;
}

Result<Ref<Entity>> EntityRegistry::create_entity(ClassHashCode class_hash)
{
    return m_entries.get(class_hash).value().c();
}

#define TEX(name) ("assets/textures/" name ".png")

void GameRegistry::register_all()
{
    add_block(Blocks::stone, newref<Block>(TEX("stone")));
    add_block(Blocks::dirt, newref<Block>(TEX("dirt")));
    add_block(Blocks::crafting_table, newref<CraftingTableBlock>());

    add_item(Items::stone_block, newref<ItemBlock>(Blocks::stone));
    add_item(Items::dirt_block, newref<ItemBlock>(Blocks::dirt));
    add_item(Items::crafting_table_block, newref<ItemBlock>(Blocks::crafting_table));
    add_item(Items::water_bucket, newref<BucketItem>());
    add_item(Items::bow, newref<BowItem>());


}

Result<void> GameRegistry::post_register()
{
    uint32_t mip_level = 1;
    m_texture_array = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureViewDimension_2DArray, m_images.size() + 1, mip_level));
    m_texture_array->update(Renderer::get().get_missing_texture_data(), 0);

    size_t index = 1;
    for (const auto& image : m_images)
    {
        m_texture_array->update(View((uint8_t *)image.data, image.w * image.h * 4), index);

        Ref<Texture> texture = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureViewDimension_2D));
        texture->update(View((uint8_t *)image.data, image.w * image.h * 4));

        // TODO: create textureview instead of duplicating data in memory.
        m_texture_handles.append(texture);
        index++;

        stbi_image_free((stbi_uc *)image.data);
    }

    // s_texture_array->generate_mips();

    for (const auto& [id, item] : m_items)
    {
        if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
        {
            Ref<Block> block = get_block(ib->block());
            ib->set_texture(create_preview_texture(block));
        }
    }

    return Result<void>();
}

void GameRegistry::add_block(Id<Block> id, Ref<Block> block)
{
    m_blocks.put(id, block);
}

void GameRegistry::add_item(Id<Item> id, Ref<Item> item)
{
    m_items.put(id, item);

    if (Ref<ItemBlock> ib = item.cast_to<ItemBlock>())
        m_block_items.put(ib->block(), id);
}

Option<Id<Block>> GameRegistry::to_block(Id<Item> id)
{
    if (!id.valid())
        return None;

    Option<Ref<Item>> item_opt = m_items.get(id);
    if (!item_opt.has_value())
        return None;

    if (Ref<ItemBlock> ib = item_opt.value().cast_to<ItemBlock>())
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
        // Ref<Item> i = item.get();
        // if (Ref<ItemBlock> ib = i.cast_to<ItemBlock>())
        // {
        //     Id<Block> block_id = ib->block();
        //     Ref<Block> b = m_blocks.get(block_id).get();
        //     return m_texture_handles.get_unchecked(b->get_texture_ids()[0]);
        // }
        return item.value()->get_texture();
    }
    return nullptr;
}

size_t GameRegistry::load_texture(const StringView& path)
{
    Option<size_t> i = get_image(path);
    if (i.has_value())
        return i.value();

    Result<File> file_opt = Filesystem::open_file(path);
    if (file_opt.has_error())
        return 0;

    LocalVector<char> buffer;
    Result<void> res = file_opt.value().reader().read_to_buffer(buffer);
    file_opt.value().close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_R(data == nullptr, format("Failed to parse image `{}`", path), 0);

    const size_t id = m_images.size() + 1;
    m_images.append(Image(data, w, h, channels, path));
    return id;
}

Ref<Texture> GameRegistry::create_texture(const StringView& path)
{
    File file = THROW(Filesystem::open_file(path), Renderer::get().get_missing_texture());
    LocalVector<char> buffer;
    THROW(file.reader().read_to_buffer(buffer), Renderer::get().get_missing_texture());
    file.close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);

    Ref<Texture> texture = THROW(Texture::create(w, h, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding), Renderer::get().get_missing_texture());
    if (data)
        texture->update(View(data, w * h * 4).as_bytes());

    stbi_image_free(data);
    return texture;
}

struct GPU_ATTRIBUTE PreviewBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

Ref<Texture> GameRegistry::create_preview_texture(Ref<Block> block)
{
    constexpr uint32_t preview_size = 128;

    Ref<Buffer> buffer = THROW(Buffer::create(sizeof(PreviewBlockModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform), Renderer::get().get_missing_texture());
    Ref<Material> material = THROW(Material::create(Renderer::get().get_preview_block_shader(), MaterialFlagBits::None, WGPUCullMode_Back, UVType::UV), Renderer::get().get_missing_texture());
    material->set_param("model", buffer);
    material->set_param("images", Engine::get().registry().get_texture_array());

    glm::mat4 matrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f) *
                       glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.f, 0.f, -0.86f)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(35.0f), glm::vec3(1, 0, 0)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(45.0f), glm::vec3(0, 1, 0));
    PreviewBlockModel model(matrix, glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16)));
    buffer->update(View(model).as_bytes());

    Ref<Texture> depth_texture = THROW(Texture::create(preview_size, preview_size, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment), Renderer::get().get_missing_texture());
    Ref<Texture> color_texture = THROW(Texture::create(preview_size, preview_size, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding), Renderer::get().get_missing_texture());

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(Renderer::get().device(), nullptr);

    WGPURenderPassDescriptor rp{};
    rp.occlusionQuerySet = nullptr;
    rp.timestampWrites = nullptr;

    WGPURenderPassColorAttachment color_attach{};
    color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attach.loadOp = WGPULoadOp_Clear;
    color_attach.storeOp = WGPUStoreOp_Store;
    color_attach.view = color_texture->handle_view();
    rp.colorAttachmentCount = 1;
    rp.colorAttachments = &color_attach;

    WGPURenderPassDepthStencilAttachment depth_attach{};
    depth_attach.depthClearValue = 1.0;
    depth_attach.depthLoadOp = WGPULoadOp_Clear;
    depth_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_attach.view = depth_texture->handle_view();
    rp.depthStencilAttachment = &depth_attach;

    WGPURenderPassEncoder render_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &rp);
    Renderer::get().draw(RenderPass(render_encoder, RenderTarget(depth_texture->format()), Vector<RenderTarget>::create(color_texture->format())), Renderer::get().get_cube_mesh(), material);
    wgpuRenderPassEncoderEnd(render_encoder);
    wgpuRenderPassEncoderRelease(render_encoder);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueSubmit(Renderer::get().get_queue(), 1, &command_buffer);

    wgpuCommandBufferRelease(command_buffer);

    return color_texture;
}

Option<size_t> GameRegistry::get_image(const StringView& path)
{
    for (size_t i = 0; i < m_images.size(); i++)
    {
        if (StringView(m_images[i].path) == path)
            return i + 1;
    }
    return None;
}

Option<ItemStack> GameRegistry::match(const InplaceVector<Id<Item>, 9>& grid, int width, int height)
{

    for (const auto& r : m_recipes)
    {

        // Skip recipes that don't fit in crafting grid. Ex: Inventory will have a limit of 2x2.
        if (r.width > width || r.height > height)
            continue;

        for (int off_y = 0; off_y <= height - r.height; off_y++)
        {
            for (int off_x = 0; off_x <= width - r.width; off_x++)
            {
                bool ok = true;
                for (int y = 0; y < r.height && ok; y++)
                {
                    // Try all possible positions where the recipe could fit.
                    for (int x = 0; x < r.width; x++)
                    {
                        Id<Item> a = grid[two_d_to_1d(off_x + x, off_y + y, width)];
                        Id<Item> b = r.pattern[two_d_to_1d(x, y, 3)];

                        if (a != b)
                        {
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok)
                    continue;

                // Ensure there is no extra item.
                for (int y = 0; y < height && ok; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        bool inside = y >= off_y && y < off_y + r.height && x >= off_x && x < off_x + r.width;

                        if (!inside && grid[two_d_to_1d(x, y, width)] != Id<Item>())
                        {
                            ok = false;
                            break;
                        }
                    }
                }

                if (ok)
                    return r.result;
            }
        }
    }

    return None;
}