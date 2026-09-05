#pragma once

#include "Core/Error.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Entity/LivingEntity.hpp"
#include "Input.hpp"
#include "Inventory/Inventory.hpp"
#include "Inventory/PlayerInventory.hpp"
#include "Model.hpp"
#include "UI/TextInput.hpp"

#include <expected>
#include "World/Biome.hpp"

enum class GameMode
{
    Survival,
    Creative,
};

class Player;

class BetterConsole
{
public:
    void process_command(Player *player, std::string_view str);

    // Commands
    void tp(Player *player, const std::vector<std::string>& args);
    void chgdim(Player *player, const std::vector<std::string>& args);
    void give(Player *player, const std::vector<std::string>& args);
    void gamemode(Player *player, const std::vector<std::string>& args);
};

class Player : public LivingEntity
{
    CLASS(Player, LivingEntity);

public:
    static void bind_methods();

    Player()
        : LivingEntity(20)
    {
        m_aabb = AABBd(-glm::dvec3(0.35, 0.9, 0.35), glm::dvec3(0.35, 0.9, 0.35));
    }

    virtual ~Player() {}

    virtual void tick(float delta) override;
    virtual void draw(const RenderPass& pass) override;
    virtual void draw_ui(const RenderPass& pass) override;
    virtual void process_event(Event& event) override;

    virtual std::expected<void, Error> save(EntitySerializer& ser) const override;
    virtual std::expected<void, Error> load(const EntitySerializer& deser) override;

    virtual void die() override;

    void set_username(std::string_view username) { m_username = username; }
    std::string_view get_username() const { return m_username; }

    void break_block(int64_t x, int64_t y, int64_t z);
    void place_block(int64_t x, int64_t y, int64_t z, glm::dvec3 normal, ItemStack stack);

    void on_ready() override;

    float get_speed() const { return m_speed; }
    void set_speed(float speed) { m_speed = speed; }

    void set_remote() { m_local_player = false; }

    void set_gamemode(GameMode gamemode) { m_gamemode = gamemode; }

    bool has_gravity() const { return m_gamemode == GameMode::Survival; }

    void set_slot(size_t slot)
    {
        m_slot = slot;
        m_inventory->set_selected_slot(slot);
    }

    std::shared_ptr<PlayerInventory> get_inventory() const { return m_inventory; }
    std::shared_ptr<InventoryContainer> get_inventory_container() const { return m_inventory_container; }

    std::shared_ptr<Camera> get_camera() const { return m_camera; }

    void open_inventory(std::shared_ptr<Inventory> inventory);
    void close_inventory();

    bool head_in_water() const;

private:
    std::shared_ptr<Camera> m_camera;
    GameMode m_gamemode = GameMode::Creative;

    float m_speed = 8.0;
    float m_sprint_speed = 14.0;

    float m_fly_speed_mult = 2.5f;

    float m_jump_force = 0.24f;

    static constexpr float head_height = 0.80;
    static constexpr float head_bobbing_max = 0.06;
    float m_target_head_height = head_bobbing_max;

    std::optional<glm::vec3> m_aimed_block;
    std::shared_ptr<Material> m_aim_material;
    std::shared_ptr<Buffer> m_aim_buffer;

    bool m_previous_frame_in_water = false;

    static constexpr glm::vec3 aim_color = glm::vec3(0.94, 0.63, 0.1);

    std::shared_ptr<ModelLegacy> m_model;
    Animator m_animator;

    std::shared_ptr<InventoryContainer> m_inventory_container;
    std::shared_ptr<PlayerInventory> m_inventory;
    size_t m_slot = 0;

    std::array<std::shared_ptr<Texture>, 4> m_breaks_textures;
    std::shared_ptr<Buffer> m_hand_model_buffer;
    std::shared_ptr<BindGroup> m_hand_item_bg;

    std::optional<std::shared_ptr<Inventory>> m_opened_inventory;

    std::string m_username;

    std::shared_ptr<Widget> m_chat;
    bool m_chat_opened = false;
    BetterConsole m_console;

    void on_text_message(TextInput& input, std::string_view message);

    /**
     * Player class is a little special since its behavior is different if this is the local or remote.
     */
    bool m_local_player = true;

    static constexpr size_t max_destroy_ticks = 35;
    size_t m_destroy_ticks = 0;
    glm::i64vec3 m_destroy_block_pos = glm::i64vec3();
    bool m_is_destroying = false;

    bool are_input_available()
    {
        return Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && !m_chat_opened;
    }

    Biome m_current_biome = Biome::None;
};
