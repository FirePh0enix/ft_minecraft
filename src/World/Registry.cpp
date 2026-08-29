#include "World/Registry.hpp"

#include "Block/Block.hpp"
#include "Block/CraftingTable.hpp"
#include "Block/Portal.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Logger.hpp"
#include "Engine.hpp"
#include "Item/Arrow.hpp"
#include "Item/Bow.hpp"
#include "Item/Bucket.hpp"
#include "Item/Crystal.hpp"
#include "Render/Renderer.hpp"

#include <memory>

constexpr int two_d_to_1d(int x, int y, int w)
{
    return y * w + x;
}

Result<std::shared_ptr<Entity>> EntityRegistry::create_entity(ClassHashCode class_hash)
{
    return m_entries[class_hash].c();
}

GameRegistry::GameRegistry()
{
    m_block_runtime_ids.push_back(Id<Block>());
}

#define TEX(name) ("assets/textures/" name ".png")
#define STRUCT(name) ("assets/structures/" name ".yml")

void GameRegistry::register_all()
{
    add_block(Blocks::stone, std::make_shared<Block>(TEX("stone")));
    add_block(Blocks::dirt, std::make_shared<Block>(TEX("dirt")));
    add_block(Blocks::sand, std::make_shared<Block>(TEX("sand")));
    add_block(Blocks::log, std::make_shared<Block>(std::array<std::string, 6>{TEX("log"), TEX("log"), TEX("log"), TEX("log"), TEX("log_top"), TEX("log_top")}));
    add_block(Blocks::leaves, std::make_shared<Block>(TEX("leaves")));
    add_block(Blocks::grass, std::make_shared<Block>(std::array<std::string, 6>{TEX("grass_side"), TEX("grass_side"), TEX("grass_side"), TEX("grass_side"), TEX("dirt"), TEX("grass_top")}, true));
    add_block(Blocks::snow, std::make_shared<Block>(TEX("snow")));
    add_block(Blocks::crafting_table, std::make_shared<CraftingTableBlock>());
    add_block(Blocks::portal, std::make_shared<PortalBlock>());

    add_item(Items::stone_block, std::make_shared<ItemBlock>(Blocks::stone));
    add_item(Items::dirt_block, std::make_shared<ItemBlock>(Blocks::dirt));
    add_item(Items::sand_block, std::make_shared<ItemBlock>(Blocks::sand));
    add_item(Items::log_block, std::make_shared<ItemBlock>(Blocks::log));
    add_item(Items::leaves_block, std::make_shared<ItemBlock>(Blocks::leaves));
    add_item(Items::grass_block, std::make_shared<ItemBlock>(Blocks::grass));
    add_item(Items::snow_block, std::make_shared<ItemBlock>(Blocks::snow));
    add_item(Items::crafting_table_block, std::make_shared<ItemBlock>(Blocks::crafting_table));
    add_item(Items::portal_block, std::make_shared<ItemBlock>(Blocks::portal));
    add_item(Items::water_bucket, std::make_shared<BucketItem>());
    add_item(Items::bow, std::make_shared<BowItem>());
    add_item(Items::arrow, std::make_shared<ArrowItem>());
    add_item(Items::crystal, std::make_shared<CrystalItem>());

    add_structure("tree", Structure::load(STRUCT("tree")));
}

Result<void> GameRegistry::post_register()
{
    uint32_t mip_level = 1;
    m_texture_array = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureDimension_2D, m_images.size() + 1, mip_level));
    m_texture_array->update(std::as_bytes(Renderer::get().get_missing_texture_data()), 0);

    size_t index = 1;
    for (const auto& image : m_images)
    {
        m_texture_array->update(std::span((std::byte *)image.data, image.w * image.h * 4), index);

        std::shared_ptr<Texture> texture = TRY(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureDimension_2D));
        texture->update(std::span((std::byte *)image.data, image.w * image.h * 4));

        // TODO: create textureview instead of duplicating data in memory.
        m_texture_handles.push_back(texture);
        index++;

        stbi_image_free((stbi_uc *)image.data);
    }

    // s_texture_array->generate_mips();

    for (const auto& [id, item] : m_items)
    {
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
        {
            std::shared_ptr<Block> block = get_block(ib->block());
            ib->set_texture(create_preview_texture(block));
        }
    }

    return Result<void>();
}

void GameRegistry::add_block(Id<Block> id, std::shared_ptr<Block> block)
{
    block->set_runtime_id(id);
    m_blocks[id] = block;

    m_block_runtime_ids.push_back(id);
    m_block_names[std::string(id.str)] = id;
}

void GameRegistry::add_item(Id<Item> id, std::shared_ptr<Item> item)
{
    m_items[id] = item;
    m_item_names[std::string(id.str)] = id;
    m_item_ids[id.hash] = id;

    if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
        m_block_items[ib->block()] = id;
}

void GameRegistry::add_structure(std::string_view name, std::shared_ptr<Structure> structure)
{
    m_structures[std::string(name)] = structure;
}

std::optional<Id<Block>> GameRegistry::to_block(Id<Item> id)
{
    if (!id.valid())
        return std::nullopt;

    std::optional<std::shared_ptr<Item>> item_opt = m_items[id];
    if (!item_opt.has_value())
        return std::nullopt;

    if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item_opt.value()))
        return ib->block();
    return std::nullopt;
}

std::shared_ptr<Texture> GameRegistry::get_texture(Id<Item> id)
{
    if (!m_items.contains(id))
    {
        warn("GameRegistry::get_texture(): invalid item Id: {}", id.hash);
        return Renderer::get().get_missing_texture();
    }

    std::shared_ptr<Item> item = m_items[id];
    return item->get_texture();
}

size_t GameRegistry::load_texture(std::string_view path)
{
    std::optional<size_t> i = get_image(path);
    if (i.has_value())
        return i.value();

    Result<File> file_opt = Filesystem::open_file(path);
    if (file_opt.has_error())
        return 0;

    std::vector<char> buffer;
    EXPECT(file_opt.value().reader().read_to_buffer(buffer));
    file_opt.value().close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_R(data == nullptr, format("Failed to parse image `{}`", path), 0);

    const size_t id = m_images.size() + 1;
    m_images.push_back(Image(data, w, h, channels, std::string(path)));
    return id;
}

std::shared_ptr<Texture> GameRegistry::create_texture(std::string_view path)
{
    File file = THROW(Filesystem::open_file(path), Renderer::get().get_missing_texture());
    std::vector<char> buffer;
    THROW(file.reader().read_to_buffer(buffer), Renderer::get().get_missing_texture());
    file.close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);

    std::shared_ptr<Texture> texture = THROW(Texture::create(w, h, WGPUTextureFormat_RGBA8UnormSrgb, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding), Renderer::get().get_missing_texture());
    if (data)
        texture->update(std::span((std::byte *)data, w * h * 4));

    stbi_image_free(data);
    return texture;
}

struct GPU_ATTRIBUTE PreviewBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

std::shared_ptr<Texture> GameRegistry::create_preview_texture(std::shared_ptr<Block> block)
{
    constexpr uint32_t preview_size = 128;

    std::shared_ptr<Buffer> buffer = THROW(Buffer::create(sizeof(PreviewBlockModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform), Renderer::get().get_missing_texture());
    std::shared_ptr<Material> material = Material::create(Renderer::get().get_preview_block_shader(), MaterialFlagBits::None, WGPUCullMode_Back, WGPUVertexFormat_Float32x2);

    std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_preview_block_shader());
    bg->set_param("model", buffer);
    bg->set_param("images", EXPECT(Engine::get().registry().get_texture_array()->get_view(WGPUTextureViewDimension_2DArray)));

    glm::mat4 matrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 10.0f) *
                       glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.f, 0.f, -0.86f)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(35.0f), glm::vec3(1, 0, 0)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(45.0f), glm::vec3(0, 1, 0));
    PreviewBlockModel model(matrix, glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16)));
    buffer->update_struct(model);

    std::shared_ptr<Texture> depth_texture = THROW(Texture::create(preview_size, preview_size, WGPUTextureFormat_Depth32Float, WGPUTextureUsage_RenderAttachment), Renderer::get().get_missing_texture());
    std::shared_ptr<Texture> color_texture = THROW(Texture::create(preview_size, preview_size, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding), Renderer::get().get_missing_texture());

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(Renderer::get().device(), nullptr);

    WGPURenderPassDescriptor rp{};
    rp.occlusionQuerySet = nullptr;
    rp.timestampWrites = nullptr;

    WGPURenderPassColorAttachment color_attach{};
    color_attach.clearValue = WGPUColor(0.0, 0.0, 0.0, 0.0);
    color_attach.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attach.loadOp = WGPULoadOp_Clear;
    color_attach.storeOp = WGPUStoreOp_Store;
    color_attach.view = EXPECT(color_texture->get_view());
    rp.colorAttachmentCount = 1;
    rp.colorAttachments = &color_attach;

    WGPURenderPassDepthStencilAttachment depth_attach{};
    depth_attach.depthClearValue = 1.0;
    depth_attach.depthLoadOp = WGPULoadOp_Clear;
    depth_attach.depthStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilLoadOp = WGPULoadOp_Clear;
    depth_attach.stencilStoreOp = WGPUStoreOp_Store;
    depth_attach.stencilClearValue = 1;
    depth_attach.view = EXPECT(depth_texture->get_view());
    rp.depthStencilAttachment = &depth_attach;

    WGPURenderPassEncoder render_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &rp);
    Renderer::get().draw(RenderPass(render_encoder, RenderTarget(depth_texture->format()), {color_texture->format()}), Renderer::get().get_cube_mesh(), material, bg);
    wgpuRenderPassEncoderEnd(render_encoder);
    wgpuRenderPassEncoderRelease(render_encoder);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueSubmit(Renderer::get().get_queue(), 1, &command_buffer);

    wgpuCommandBufferRelease(command_buffer);

    return color_texture;
}

std::optional<size_t> GameRegistry::get_image(std::string_view path)
{
    for (size_t i = 0; i < m_images.size(); i++)
    {
        if (m_images[i].path == path)
            return i + 1;
    }
    return std::nullopt;
}

std::optional<ItemStack> GameRegistry::match(const std::array<Id<Item>, MAX_RECIPE_SIZE>& grid, int width, int height)
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

    return std::nullopt;
}

std::optional<RpcTarget> GameRegistry::get_rpc(Entity *entity, std::string_view name) const
{
    for (ssize_t i = (ssize_t)entity->get_classes().size() - 1; i >= 0; i--)
    {
        ClassHashCode class_hash = entity->get_classes()[i];
        auto rpcs = m_exposed_rpc.find(class_hash);
        if (rpcs != m_exposed_rpc.end())
        {
            auto target = rpcs->second.find(name);
            if (target != rpcs->second.end())
                return target->second;
        }
    }
    return std::nullopt;
}
