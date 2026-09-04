#pragma once

#include "Block/Block.hpp"
#include "Entity/Entity.hpp"
#include "Item/Item.hpp"
#include "Item/ItemStack.hpp"
#include "Render/Renderer.hpp"
#include "Resource/BlockState.hpp"
#include "Resource/Model.hpp"
#include "Structure.hpp"

#include <memory>
#include <stb_image.h>

struct AtlasTexture
{
    const stbi_uc *data;
    int w;
    int h;
    int channels;
    std::string path;
};

struct AtlasTextureData
{
    int64_t x;
    int64_t y;
    int64_t width;
    int64_t height;
};

constexpr int MAX_RECIPE_SIZE = 9;

struct Recipe
{
    uint8_t width = 3;
    uint8_t height = 3;

    std::array<Id<Item>, MAX_RECIPE_SIZE> pattern;
    ItemStack result;
};

template <typename T>
concept HasBindMethods = requires(T a) {
    { T::bind_methods() } -> std::convertible_to<void>;
};

class EntityRegistry
{
public:
    using Constructor = std::shared_ptr<Entity> (*)();

    struct Entry
    {
        Constructor c;
    };

    template <typename T>
        requires(HasBindMethods<T>)
    void register_entity()
    {
        // T::bind_methods();
        m_entries[T::get_static_hash_code()] = Entry{.c = []() -> std::shared_ptr<Entity>
                                                     { return std::make_shared<T>(); }};
    }

    template <typename T>
    void register_entity()
    {
        m_entries[T::get_static_hash_code()] = Entry{.c = []() -> std::shared_ptr<Entity>
                                                     { return std::make_shared<T>(); }};
    }

    Result<std::shared_ptr<Entity>> create_entity(ClassHashCode class_hash);

private:
    std::map<ClassHashCode, Entry> m_entries;
};

namespace Blocks
{
constexpr Id<Block> stone("stone");
constexpr Id<Block> dirt("dirt");
constexpr Id<Block> sand("sand");
constexpr Id<Block> grass_block("grass_block");
constexpr Id<Block> snow_block("snow_block");

constexpr Id<Block> oak_log("oak_log");
constexpr Id<Block> oak_leaves("oak_leaves");

constexpr Id<Block> grass("grass");

// constexpr Id<Block> crafting_table("crafting_table");
// constexpr Id<Block> portal("portal");
// constexpr Id<Block> dandelion("dandelion");
} // namespace Blocks

namespace Items
{
constexpr Id<Item> stone("stone");
constexpr Id<Item> dirt("dirt");
constexpr Id<Item> sand("sand");
constexpr Id<Item> oak_log("log");
constexpr Id<Item> oak_leaves("leaves");
constexpr Id<Item> grass_block("grass");
constexpr Id<Item> snow("snow");

constexpr Id<Item> grass("grass");

// constexpr Id<Item> crafting_table_block("crafting_table");
// constexpr Id<Item> portal_block("portal");
// constexpr Id<Item> water_bucket("water_bucket");
// constexpr Id<Item> bow("bow");
// constexpr Id<Item> arrow("arrow");
// constexpr Id<Item> crystal("crystal");
// constexpr Id<Item> dandelion("dandelion");
}; // namespace Items

namespace Entities
{
constexpr Id<Entity> player("player");
constexpr Id<Entity> cow("cow");
constexpr Id<Entity> zombie("zombie");
}; // namespace Entities

constexpr int64_t atlas_size = 512;

class GameRegistry
{
public:
    GameRegistry();

    void register_all();
    Result<void> post_register();

    Result<BlockStateResource> get_blockstate(std::string_view path);
    Result<Model> get_model(std::string_view path);
    Result<void> add_tint(std::string_view path);

    void register_block(Id<Block> id, bool collision = true);
    void register_block(Id<Block> id, std::shared_ptr<Block> block);

    void add_item(Id<Item> id, std::shared_ptr<Item> item);
    void add_structure(std::string_view name, std::shared_ptr<Structure> structure);

    std::shared_ptr<Block> get_block(Id<Block> key) const
    {
        if (key.hash == 0)
            return nullptr;
        return m_blocks.at(key);
    }
    std::shared_ptr<Block> get_block(uint32_t key) const
    {
        if (key == 0)
            return nullptr;
        return m_blocks.at(Id<Block>(key));
    }

    std::shared_ptr<Item> get_item(Id<Item> key) const
    {
        return m_items.at(key);
    }
    std::shared_ptr<Structure> get_struct(std::string_view name) const { return m_structures.find(name)->second; }

    Id<Item> item_from_name(std::string_view name) const
    {
        auto iter = m_item_names.find(name);
        if (iter == m_item_names.end())
            return Id<Item>();
        return iter->second;
    }

    Id<Item> item_from_id(uint32_t id) const
    {
        auto iter = m_item_ids.find(id);
        if (iter == m_item_ids.end())
            return Id<Item>();
        return iter->second;
    }

    Id<Block> block_from_name(std::string_view name) const
    {
        auto iter = m_block_names.find(name);
        if (iter == m_block_names.end())
            return Id<Block>();
        return iter->second;
    }

    std::optional<Id<Block>> to_block(Id<Item> id);
    std::optional<Id<Item>> to_item(Id<Block> block) { return m_block_items[block]; }
    std::optional<Id<Item>> to_item(uint32_t block) { return m_block_items[Id<Block>(block)]; }

    Id<Block> get_block_id(std::string_view name)
    {
        auto iter = m_block_names.find(name);
        if (iter == m_block_names.end())
            return Id<Block>();
        return iter->second;
    }

    std::shared_ptr<Block> block_from_item(Id<Item> key)
    {
        std::shared_ptr<Item> item = m_items[key];
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
            return m_blocks[ib->block()];
        return nullptr;
    }

    std::shared_ptr<Texture> get_texture(Id<Item> item);

    AtlasTextureData get_atlas_data(std::string_view path) const
    {
        auto iter = m_atlas_data.find(path);
        if (iter == m_atlas_data.end()) [[unlikely]]
            return {};
        return iter->second;
    }

    std::shared_ptr<Texture> get_atlas() const { return m_atlas; }
    float get_atlas_size() const { return static_cast<float>(atlas_size); }

    std::shared_ptr<Texture> get_tintmap() const { return m_tint_texture_array; }

    Result<void> add_texture(std::string_view path);

    /**
     * Create a texture from a file path. If an error occurs the missing texture is returned.
     */
    std::shared_ptr<Texture> create_texture(std::string_view path);

    /**
     * Render a 3D preview of the block. If an error occurs the missing texture is returned.
     */
    std::shared_ptr<Texture> create_preview_texture(std::shared_ptr<Block> block);

    void register_rpc(ClassHashCode cls, std::string_view name, RpcTarget target)
    {
        m_exposed_rpc[cls][std::string(name)] = target;
    }

    std::optional<RpcTarget> get_rpc(Entity *entity, std::string_view name) const;

    // Crafting system.
    void add_recipe(const Recipe& recipe) { m_recipes.push_back(recipe); }
    std::optional<ItemStack> match(const std::array<Id<Item>, 9>& grid, int width, int height);

private:
    stdext::string_map<BlockStateResource> m_blockstates;
    stdext::string_map<Model> m_models;

    std::map<Id<Block>, std::shared_ptr<Block>> m_blocks;
    std::map<Id<Item>, std::shared_ptr<Item>> m_items;

    stdext::string_map<std::shared_ptr<Structure>> m_structures;

    std::map<Id<Block>, Id<Item>> m_block_items;

    std::vector<Id<Block>> m_block_runtime_ids;
    stdext::string_map<Id<Block>> m_block_names;

    stdext::string_map<Id<Item>> m_item_names;
    std::map<uint32_t, Id<Item>> m_item_ids;

    stdext::string_map<AtlasTexture> m_textures;
    std::shared_ptr<Texture> m_atlas;
    stdext::string_map<AtlasTextureData> m_atlas_data;

    std::vector<AtlasTexture> m_tint_textures;
    std::shared_ptr<Texture> m_tint_texture_array;

    std::map<ClassHashCode, stdext::string_map<RpcTarget>> m_exposed_rpc;

    std::vector<Recipe> m_recipes;
};
