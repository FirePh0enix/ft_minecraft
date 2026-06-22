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
constexpr Id<Block> stone(1);
constexpr Id<Block> dirt(2);
constexpr Id<Block> sand(3);
constexpr Id<Block> log(4);
constexpr Id<Block> leaves(5);
constexpr Id<Block> grass(6);
constexpr Id<Block> snow(7);
constexpr Id<Block> crafting_table(8);
} // namespace Blocks

namespace Items
{
constexpr Id<Item> stone_block(1);
constexpr Id<Item> dirt_block(2);
constexpr Id<Item> sand_block(3);
constexpr Id<Item> log_block(4);
constexpr Id<Item> leaves_block(5);
constexpr Id<Item> grass_block(6);
constexpr Id<Item> snow_block(7);
constexpr Id<Item> crafting_table_block(8);
constexpr Id<Item> water_bucket(9);
constexpr Id<Item> bow(5);
constexpr Id<Item> arrow(6);

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

    void add_block(Id<Block> id, std::shared_ptr<Block> block);
    void add_item(Id<Item> id, std::shared_ptr<Item> item);
    void add_structure(std::string_view name, std::shared_ptr<Structure> structure);

    std::shared_ptr<Block> get_block(Id<Block> key) const { return m_blocks.at(key); }
    std::shared_ptr<Item> get_item(Id<Item> key) const { return m_items.at(key); }
    std::shared_ptr<Structure> get_struct(std::string_view name) const { return m_structures.find(name)->second; }

    std::optional<Id<Block>> to_block(Id<Item> id);
    std::optional<Id<Item>> to_item(Id<Block> block) { return m_block_items[block]; }
    std::optional<Id<Block>> item_from_name(std::string_view name) { return m_block_names.find(name)->second; }

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
    std::map<uint16_t, Id<Block>> m_block_ids;
    stdext::string_map<Id<Block>> m_block_names;

    std::vector<Image> m_images;
    std::shared_ptr<Texture> m_texture_array;
    std::vector<std::shared_ptr<Texture>> m_texture_handles;

    std::map<ClassHashCode, stdext::string_map<RpcTarget>> m_exposed_rpc;

    std::optional<size_t> get_image(std::string_view path);

    std::vector<Recipe> m_recipes;
};
