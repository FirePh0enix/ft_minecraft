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
#include "webgpu/webgpu.h"

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

void GameRegistry::register_all()
{
    EXPECT(add_tint("colormap/grass"));

    register_block(Blocks::stone);
    register_block(Blocks::dirt);
    register_block(Blocks::grass_block);
    register_block(Blocks::sand);
    register_block(Blocks::snow_block);

    register_block(Blocks::oak_leaves);
    register_block(Blocks::oak_log);

    register_block(Blocks::grass);

    // add_block(Blocks::crafting_table, std::make_shared<CraftingTableBlock>());
    // add_block(Blocks::portal, std::make_shared<PortalBlock>());
    // add_block(Blocks::dandelion, std::make_shared<PlantBlock>("data/resourcepacks/core/assets/minecraft/textures/block/dandelion.png"));

    add_item(Items::stone, std::make_shared<ItemBlock>(Blocks::stone));
    add_item(Items::dirt, std::make_shared<ItemBlock>(Blocks::dirt));
    add_item(Items::sand, std::make_shared<ItemBlock>(Blocks::sand));
    add_item(Items::grass_block, std::make_shared<ItemBlock>(Blocks::grass_block));
    add_item(Items::snow, std::make_shared<ItemBlock>(Blocks::snow_block));

    add_item(Items::oak_log, std::make_shared<ItemBlock>(Blocks::oak_log));
    add_item(Items::oak_leaves, std::make_shared<ItemBlock>(Blocks::oak_leaves));

    add_item(Items::grass,  std::make_shared<ItemBlock>(Blocks::grass));

    // add_item(Items::crafting_table_block, std::make_shared<ItemBlock>(Blocks::crafting_table));
    // add_item(Items::portal_block, std::make_shared<ItemBlock>(Blocks::portal));
    // add_item(Items::water_bucket, std::make_shared<BucketItem>());
    // add_item(Items::bow, std::make_shared<BowItem>());
    // add_item(Items::arrow, std::make_shared<ArrowItem>());
    // add_item(Items::crystal, std::make_shared<CrystalItem>());
}

Result<void> GameRegistry::post_register()
{
    // TODO: try to guess the most optimal size.
    m_atlas = TRY(Texture::create(atlas_size, atlas_size, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureDimension_2D));
    uint32_t *pixels = new uint32_t[atlas_size * atlas_size];

    int64_t max_height = 0;

    int64_t x = 0;
    int64_t y = 0;
    for (const auto& [path, texture] : m_textures)
    {
        if (x + texture.w > atlas_size)
        {
            x = 0;
            y += max_height;
        }

        for (int64_t xx = 0; xx < texture.w; xx++)
            for (int64_t yy = 0; yy < texture.h; yy++)
            {
                pixels[(xx + x) + (yy + y) * atlas_size] = ((uint32_t *)texture.data)[xx + yy * texture.w];
            }

        AtlasTextureData data{};
        data.x = x;
        data.y = y;
        data.width = texture.w;
        data.height = texture.h;
        m_atlas_data[path] = data;

        if (texture.h > max_height)
            max_height = texture.h;
        x += texture.w;

        stbi_image_free((stbi_uc *)texture.data);
    }

    m_atlas->update(std::span<std::byte>((std::byte *)pixels, atlas_size * atlas_size * 4));
    delete[] pixels;

    for (const auto& [id, item] : m_items)
    {
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
        {
            std::shared_ptr<Block> block = get_block(ib->block());
            ib->set_texture(create_preview_texture(block));
        }
    }

    m_tint_texture_array = TRY(Texture::create(m_tint_textures[0].w, m_tint_textures[0].h, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureDimension_2D, m_tint_textures.size()));
    for (size_t i = 0; i < m_tint_textures.size(); i++)
    {
        const AtlasTexture& texture = m_tint_textures[i];
        m_tint_texture_array->update(std::span<std::byte>((std::byte *)texture.data, texture.w * texture.h * 4), i);
        stbi_image_free((stbi_uc *)texture.data);
    }

    return Result<void>();
}

Result<BlockStateResource> GameRegistry::get_blockstate(std::string_view path)
{
    auto iter = m_blockstates.find(path);
    if (iter != m_blockstates.end())
        return iter->second;

    std::string pathf = "data/resourcepacks/core/assets/minecraft/blockstates/";
    pathf += path;
    pathf += ".json";

    Result<File> file_res = Filesystem::open_file(pathf);
    if (file_res.has_error())
    {
        error("cannot open file `{}`", pathf);
        return file_res.error();
    }
    File file = file_res.value();
    std::string text = TRY(file.reader().read_to_string());
    file.close();

    BlockStateResource blockstate = nlohmann::json::parse(text);

    m_blockstates[std::string(path)] = blockstate;
    return blockstate;
}

Result<Model> GameRegistry::get_model(std::string_view path)
{
    auto iter = m_models.find(path);
    if (iter != m_models.end())
        return iter->second;

    std::string pathf = "data/resourcepacks/core/assets/minecraft/models/";
    pathf += path;
    pathf += ".json";

    Result<File> file_res = Filesystem::open_file(pathf);
    if (file_res.has_error())
    {
        error("cannot open file `{}`", pathf);
        return file_res.error();
    }
    File file = file_res.value();
    std::string text = TRY(file.reader().read_to_string());
    file.close();

    Model model = nlohmann::json::parse(text);
    if (model.parent.has_value())
    {
        // TODO: detect circular dependency.
        Model parent = TRY(get_model(model.parent.value()));
        model.resolve(parent);
    }
    else
    {
        model.resolve();
    }

    for (const auto& [name, ref] : model.textures)
    {
        if (!ref.starts_with("#"))
            TRY(add_texture(ref));
    }

    m_models[std::string(path)] = model;
    return model;
}

Result<void> GameRegistry::add_tint(std::string_view path)
{
    std::string pathf = "data/resourcepacks/core/assets/minecraft/textures/";
    pathf += path;
    pathf += ".png";

    Result<File> file_opt = Filesystem::open_file(pathf);
    if (file_opt.has_error())
        return Result<void>();

    std::vector<char> buffer;
    EXPECT(file_opt.value().reader().read_to_buffer(buffer));
    file_opt.value().close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_R(data == nullptr, format("Failed to parse image `{}`", path), Result<void>());

    m_tint_textures.push_back(AtlasTexture(data, w, h, channels, std::string(path)));
    return Result<void>();
}

void GameRegistry::register_block(Id<Block> id)
{
    m_blocks[id] = std::make_shared<Block>(id.str);

    m_block_runtime_ids.push_back(id);
    m_block_names[std::string(id.str)] = id;
}

void GameRegistry::register_block(Id<Block> id, std::shared_ptr<Block> block)
{
    m_blocks[id] = block;

    m_block_runtime_ids.push_back(id);
    m_block_names[std::string(id.str)] = id;
}

Result<void> GameRegistry::add_texture(std::string_view path)
{
    if (m_textures.contains(path))
        return Result<void>();

    // TODO: allow overrides.
    std::string pathf = "data/resourcepacks/core/assets/minecraft/textures/";
    pathf += path;
    pathf += ".png";

    Result<File> file_opt = Filesystem::open_file(pathf);
    if (file_opt.has_error())
        return Result<void>();

    std::vector<char> buffer;
    EXPECT(file_opt.value().reader().read_to_buffer(buffer));
    file_opt.value().close();

    int w, h, channels;
    stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
    ERR_COND_R(data == nullptr, format("Failed to parse image `{}`", path), Result<void>());

    m_textures[std::string(path)] = (AtlasTexture(data, w, h, channels, std::string(path)));
    return Result<void>();
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

std::shared_ptr<Texture> GameRegistry::create_preview_texture(std::shared_ptr<Block> block)
{
    constexpr uint32_t preview_size = 128;

    NeighborFlags flags{};
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> uvs;
    std::vector<glm::vec3> normals;
    block->add({0, 0, 0}, flags, indices, vertices, uvs, normals);
    std::shared_ptr<Mesh> mesh = THROW(Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, normals, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint32, WGPUVertexFormat_Float32x4), Renderer::get().get_missing_texture());

    std::shared_ptr<Buffer> model_buffer = THROW(Buffer::create(sizeof(FwModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform), Renderer::get().get_missing_texture());
    glm::mat4 matrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 10.0f) *
                       glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.f, 0.f, -0.86f)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(35.0f), glm::vec3(1, 0, 0)) *
                       glm::rotate(glm::identity<glm::mat4>(), glm::radians(45.0f), glm::vec3(0, 1, 0));
    FwModel model(matrix);
    model_buffer->update_struct(model);

    std::shared_ptr<Buffer> camera_buffer = THROW(Buffer::create(sizeof(FwModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform), Renderer::get().get_missing_texture());
    FwCamera camera(glm::identity<glm::mat4>());
    camera_buffer->update_struct(camera);

    std::shared_ptr<BindGroup> bg = BindGroup::create(Renderer::get().get_model_noshadow_shader());
    bg->set_param("model", model_buffer);
    bg->set_param("camera", camera_buffer);
    bg->set_param("world_env", Renderer::get().get_fw_world_env());
    bg->set_param("atlas", EXPECT(Engine::get().registry().get_atlas()->get_view()));

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
    Renderer::get().draw(RenderPass(render_encoder, RenderTarget(depth_texture->format()), {color_texture->format()}), mesh, Renderer::get().get_model_noshadow_mat(), bg);
    wgpuRenderPassEncoderEnd(render_encoder);
    wgpuRenderPassEncoderRelease(render_encoder);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);

    std::lock_guard<std::mutex> guard(Renderer::get().get_queue_mutex());
    wgpuQueueSubmit(Renderer::get().get_queue(), 1, &command_buffer);

    wgpuCommandBufferRelease(command_buffer);

    return color_texture;
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
