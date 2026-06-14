#pragma once

#include "Core/Containers/Array.hpp"
#include "Core/Ref.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Entity/LivingEntity.hpp"
#include "Inventory/Inventory.hpp"
#include "Inventory/PlayerInventory.hpp"
#include "Model.hpp"
#include "UI/Chat.hpp"
#include "UI/Container.hpp"

enum class GameMode
{
    Survival,
    Creative,
};

class Player;

class DebugMenuContainer : public Container
{
    CLASS(DebugMenuContainer, Container);

public:
    DebugMenuContainer(Player *player);

    virtual void update(float d) override;

private:
    Player *m_player;

    Ref<Label> m_memory_label;
    Ref<Label> m_gpu_objects_label;
    Ref<Label> m_perfomance_label;
    Ref<Label> m_time_label;
    Ref<Label> m_position_label;
};

class Player : public LivingEntity
{
    CLASS(Player, LivingEntity);

public:
    //  health, attack_damage, speed, jump_force
    Player()
        : LivingEntity(20)
    {
        m_aabb = AABB(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    virtual ~Player() {}

    virtual void tick(float delta) override;
    virtual void draw(const RenderPass& pass) override;
    virtual void draw_ui(const RenderPass& pass) override;
    virtual void process_event(Event& event) override;

    virtual void save(EntitySerializer& ser) const override;
    virtual void load(const EntitySerializer& deser) override;

    virtual void die() override;

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

    Ref<PlayerInventory> get_inventory() const { return m_inventory; }
    Ref<InventoryContainer> get_inventory_container() const { return m_inventory_container; }

    void open_inventory(Ref<Inventory> inventory);
    void close_inventory();

private:
    Ref<Camera> m_camera;
    GameMode m_gamemode = GameMode::Survival;

    float m_speed = 8.0;
    float m_jump_force = 0.24f;

    Option<glm::vec3> m_aimed_block;
    Ref<Material> m_aim_material;
    Ref<Buffer> m_aim_buffer;

    bool m_previous_frame_in_water = false;

    static constexpr glm::vec3 aim_color = glm::vec3(0.94, 0.63, 0.1);

    Ref<Model> m_model;
    Animator m_animator;

    Ref<InventoryContainer> m_inventory_container;
    Ref<PlayerInventory> m_inventory;
    size_t m_slot = 0;

    Array<Ref<Texture>, 4> m_breaks_textures;
    Ref<Buffer> m_model_buffer;
    Ref<Material> m_material;

    Option<Ref<Inventory>> m_opened_inventory;

    bool m_open_chat = false;
    Ref<Chat> m_chat;

    bool m_open_debug_menu = false;
    Ref<DebugMenuContainer> m_debug_menu;

    /**
     * Player class is a little special since its behavor is different if this is the local or remote.
     */
    bool m_local_player = true;

    static constexpr size_t max_destroy_ticks = 35;
    size_t m_destroy_ticks = 0;
    glm::i64vec3 m_destroy_block_pos = glm::i64vec3();
    bool m_is_destroing = false;

    bool are_input_available() { return Input::is_mouse_grabbed() && !m_opened_inventory.has_value() && !m_open_chat; }
};
