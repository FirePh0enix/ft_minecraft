#pragma once

#include "Block/Block.hpp"
#include "Entity/Entity.hpp"
#include "Item/Item.hpp"
#include "Item/ItemStack.hpp"
#include "Render/Renderer.hpp"
#include "Structure.hpp"

#include <memory>
#include <stb_image.h>

struct Image
{
    const stbi_uc *data;
    int w;
    int h;
    int channels;
    std::string path;
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
constexpr Id<Block> log("log");
constexpr Id<Block> leaves("leaves");
constexpr Id<Block> grass("grass");
constexpr Id<Block> snow("snow");
constexpr Id<Block> crafting_table("crafting_table");
} // namespace Blocks

namespace Items
{
constexpr Id<Item> stone_block("stone");
constexpr Id<Item> dirt_block("dirt");
constexpr Id<Item> sand_block("sand");
constexpr Id<Item> log_block("log");
constexpr Id<Item> leaves_block("leaves");
constexpr Id<Item> grass_block("grass");
constexpr Id<Item> snow_block("snow");
constexpr Id<Item> crafting_table_block("crafting_table");
constexpr Id<Item> water_bucket("water_bucket");
constexpr Id<Item> bow("bow");
constexpr Id<Item> arrow("arrow");

}; // namespace Items

namespace Entities
{
constexpr Id<Entity> player("player");
constexpr Id<Entity> cow("cow");
constexpr Id<Entity> zombie("zombie");
}; // namespace Entities

class GameRegistry
{
public:
    GameRegistry();

    void register_all();
    Result<void> post_register();

    void add_block(Id<Block> id, std::shared_ptr<Block> block);
    void add_item(Id<Item> id, std::shared_ptr<Item> item);
    void add_structure(std::string_view name, std::shared_ptr<Structure> structure);

    std::shared_ptr<Block> get_block(Id<Block> key) const { return m_blocks.at(key); }

    std::shared_ptr<Block> get_block(RuntimeId<Block> key) const
    {
        Id<Block> id = from_runtime_id(key);
        if (!id.valid())
            return nullptr;
        return m_blocks.at(id);
    }

    std::shared_ptr<Item> get_item(Id<Item> key) const
    {
        return m_items.at(key);
    }
    std::shared_ptr<Structure> get_struct(std::string_view name) const { return m_structures.find(name)->second; }

    Id<Block> from_runtime_id(RuntimeId<Block> id) const
    {
        if (id.value >= m_block_runtime_ids.size())
            return Id<Block>();
        return m_block_runtime_ids[id.value];
    }

    RuntimeId<Block> get_runtime_id(Id<Block> block) const { return m_block_ids.at(block); }
    RuntimeId<Block> get_runtime_id(std::string_view block) const { return get_runtime_id(block_from_name(block)); }

    Id<Item> item_from_name(std::string_view name) const { return m_item_names.find(name)->second; }
    Id<Block> block_from_name(std::string_view name) const { return m_block_names.find(name)->second; }

    std::optional<Id<Block>> to_block(Id<Item> id);
    std::optional<Id<Item>> to_item(Id<Block> block) { return m_block_items[block]; }

    BlockState get_default_state(Id<Block> id) const { return get_block(id)->get_default_state(); }

    std::shared_ptr<Block> block_from_item(Id<Item> key)
    {
        std::shared_ptr<Item> item = m_items[key];
        if (std::shared_ptr<ItemBlock> ib = std::dynamic_pointer_cast<ItemBlock>(item))
            return m_blocks[ib->block()];
        return nullptr;
    }

    std::shared_ptr<Texture> get_texture(Id<Item> item);

    std::shared_ptr<Texture> get_texture_array() const { return m_texture_array; }

    size_t load_texture(std::string_view path);

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
    std::map<Id<Block>, std::shared_ptr<Block>> m_blocks;
    std::map<Id<Item>, std::shared_ptr<Item>> m_items;

    stdext::string_map<std::shared_ptr<Structure>> m_structures;

    std::map<Id<Block>, Id<Item>> m_block_items;

    std::vector<Id<Block>> m_block_runtime_ids;
    std::map<Id<Block>, RuntimeId<Block>> m_block_ids;
    stdext::string_map<Id<Block>> m_block_names;

    stdext::string_map<Id<Item>> m_item_names;

    std::vector<Image> m_images;
    std::shared_ptr<Texture> m_texture_array;
    std::vector<std::shared_ptr<Texture>> m_texture_handles;

    std::map<ClassHashCode, stdext::string_map<RpcTarget>> m_exposed_rpc;

    std::optional<size_t> get_image(std::string_view path);

    std::vector<Recipe> m_recipes;
};
