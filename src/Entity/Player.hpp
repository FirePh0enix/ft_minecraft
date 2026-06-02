#pragma once

#include "Core/Containers/Array.hpp"
#include "Core/Ref.hpp"
#include "Entity/Camera.hpp"
#include "Entity/Entity.hpp"
#include "Entity/Mob.hpp"
#include "Inventory/Inventory.hpp"
#include "Inventory/PlayerInventory.hpp"
#include "Model.hpp"
#include "Render/Graph.hpp"

enum class GameMode
{
    Survival,
    Creative,
};

class Player : public Mob
{
    CLASS(Player, Mob);

public:
    Player();

    virtual ~Player() {}

    virtual void tick(float delta) override;
    virtual void draw(const RenderPassNode& node) override;
    virtual void draw_ui(const RenderPassNode& node) override;

    virtual void save(EntitySerializer& ser) const override;
    virtual void load(const EntitySerializer& deser) override;

    void on_ready() override;

    float get_speed() const { return m_speed; }
    void set_speed(float speed) { m_speed = speed; }

    void set_remote() { m_local_player = false; }

    void set_gamemode(GameMode gamemode) { m_gamemode = gamemode; }

    virtual void on_hit_by(Entity& entity) override;

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

protected:
    void die() override;

private:
    Ref<Camera> m_camera;
    GameMode m_gamemode = GameMode::Survival;

    float m_speed = 8.0;
    float m_jump_force = 0.24f;

    Option<glm::vec3> m_aimed_block;
    Ref<Material> m_aim_material;
    Ref<Buffer> m_aim_buffer;

    static constexpr glm::vec3 aim_color = glm::vec3(0.94, 0.63, 0.1);

    Ref<Model> m_model;
    Animator m_animator;

    Ref<InventoryContainer> m_inventory_container;
    Ref<PlayerInventory> m_inventory;
    size_t m_slot = 0;

    Array<Ref<Texture>, 4> m_breaks_textures;
    Ref<Buffer> m_model_buffer;
    Ref<Material> m_material;

    // bool m_open_inventory = false;
    Option<Ref<Inventory>> m_opened_inventory;

    /**
     * Player class is a little special since its behavor is different if this is the local or remote.
     */
    bool m_local_player = true;

    static constexpr size_t max_destroy_ticks = 35;
    size_t m_destroy_ticks = 0;
    glm::i64vec3 m_destroy_block_pos = glm::i64vec3();
    bool m_is_destroing = false;

    bool are_input_available() { return Input::is_mouse_grabbed() && !m_opened_inventory.has_value(); }
};
