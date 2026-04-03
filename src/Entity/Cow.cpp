#include "Cow.hpp"
#include "Core/Print.hpp"
#include "Entity/Entity.hpp"
#include "Render/Driver.hpp"
#include "World/World.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>

static Ref<Mesh> create_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    std::array<uint16_t, 36> indices{
        0, 1, 2,
        2, 3, 0, // front

        20, 21, 22,
        22, 23, 20, // back

        4, 5, 6,
        6, 7, 4, // right

        12, 13, 14,
        14, 15, 12, // left

        8, 9, 10,
        10, 11, 8, // top

        16, 17, 18,
        18, 19, 16, // bottom
    };
    // clang-format on

    std::array<glm::vec3, 24> vertices{
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // front
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // back
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // left
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // right
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z), // top
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // bottom
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
    };

    std::array<glm::vec2, 24> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    std::array<glm::vec3, 24> normals{
        glm::vec3(0.0, 0.0, 1.0), // front
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),

        glm::vec3(0.0, 0.0, -1.0), // back
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),

        glm::vec3(1.0, 0.0, 0.0), // left
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),

        glm::vec3(-1.0, 0.0, 0.0), // right
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),

        glm::vec3(0.0, 1.0, 0.0), // top
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),

        glm::vec3(0.0, -1.0, 0.0), // bottom
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };

    View<uint16_t> indices_span = indices;

    return Mesh::create_from_data(indices_span.as_bytes(), vertices, normals, View(uvs).as_bytes(), IndexType::Uint16);
}

void Cow::start() {};

void Cow::tick(float delta)
{

    size_t index = m_pathfinding->m_path_index;

    if (index < m_pathfinding->m_path.size())
    {
        const size_t last_index = m_pathfinding->m_path.size() - 1;
        const size_t next_index = std::clamp(index + 1, index, last_index);

        const glm::vec3 cow_pos = get_global_transform().position();
        const glm::ivec3 cow_grid_pos = glm::ivec3(std::round(cow_pos.x), std::round(cow_pos.y), std::round(cow_pos.z));

        // Ensure actual node and next node are still walkable, else find a new path.
        if (!m_pathfinding->is_walkable(m_pathfinding->m_path[index]->m_position, 1) || !m_pathfinding->is_walkable(m_pathfinding->m_path[next_index]->m_position, 1))
        {
            // Target is still last node.
            const glm::vec3 target_pos = m_pathfinding->m_path[last_index]->m_position;
            m_pathfinding->find_path(cow_grid_pos, target_pos);
            println("Old path cannot be reached, calculating new one: {} {} {}", target_pos.x, target_pos.y, target_pos.z);
            index = m_pathfinding->m_path_index;
        }

        Node *next_node = m_pathfinding->m_path[index];
        glm::vec3 dir = next_node->m_position - cow_pos;
        glm::vec3 horizontal_dir = dir;
        horizontal_dir.y = 0.0f;

        float dist = glm::length2(horizontal_dir);

        if (dist > 0.0001f)
        {
            glm::vec3 direction = glm::normalize(horizontal_dir);
            m_velocity.x = direction.x * m_speed * delta;
            m_velocity.z = direction.z * m_speed * delta;
        }
        else
            m_pathfinding->m_path_index++;

        // TODO: Can be better.
        bool should_jump = std::round(next_node->m_position.y - cow_pos.y) >= 1.0f && std::round(dir.y) >= 1.0f;

        if (!m_jumping && should_jump && m_on_ground)
        {
            m_velocity.y = m_speed * 2.0f;
            m_jumping = true;
        }
    }

    if (!m_on_ground)
        // Realistic gravity make him spawm half in ground.
        // m_velocity.y -= 9.81f * delta;
        m_velocity.y -= 2.0f * delta;

    else
        m_jumping = false;

    move_and_collide(true);

    m_velocity.x = 0.0f;
    m_velocity.y = std::min(m_velocity.x, 40.0f);
    m_velocity.z = 0.0f;
}

void Cow::draw(RenderPassEncoder& encoder)
{

    m_model.model_matrix = m_transform.to_matrix();
    m_model_buffer->update(View(m_model).as_bytes());

    encoder.bind_material(m_material);
    encoder.bind_index_buffer(m_mesh->get_buffer(MeshBufferKind::Index));
    encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Position), 0);
    encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::Normal), 1);
    encoder.bind_vertex_buffer(m_mesh->get_buffer(MeshBufferKind::UV), 2);

    encoder.draw(m_mesh->vertex_count(), 1);
}

void Cow::on_ready()
{

    m_mesh = create_cube_mesh(glm::vec3(1.0f));
    m_aabb = AABB(glm::vec3(), glm::vec3(0.5f, 0.5f, 0.5f));

    m_model_buffer = RenderingDriver::get()
                         ->create_buffer(sizeof(Model), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest)
                         .value_or(nullptr);

    m_shader = Shader::load("assets/shaders/cow.wgsl").value_or(nullptr);

    m_shader->set_binding("env", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 0, BindingAccess::Read));
    m_shader->set_binding("model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 1, BindingAccess::Read));

    m_material = RenderingDriver::get()->create_material(m_shader, std::nullopt, MaterialFlagBits::None, PolygonMode::Fill, CullMode::Back, UVType::UVT, "COW_MATERIAL");
    m_material->set_param("env", m_world->get_env_buffer());
    m_material->set_param("model", m_model_buffer);

    register_rpc("on_hit", [this](Entity& attacker)
                 { this->on_hit_by(attacker); }, RpcTarget::SERVER);

    m_id = World::next_id();

    m_pathfinding = new Pathfinding(m_world);
}

void Cow::die()
{
    m_active = false;
    println("Cow died !");
}

void Cow::on_hit_by(Entity& entity)
{
    Mob *mob_caller = dynamic_cast<Mob *>(&entity);

    int damage = mob_caller->get_attack_damage();
    // take_damage(damage);

    if (m_health <= 0)
    {
        die();
        return;
    }

    flee_from(entity, 10);
}

static glm::ivec3 find_flee_target(
    Pathfinding& pathfinder,
    const glm::ivec3& start,
    const glm::ivec3& threat,
    int radius)
{
    glm::ivec3 best = start;
    float best_score = -FLT_MAX;

    for (int i = 0; i < 16; i++)
    {
        int dx = rand() % (radius * 2 + 1) - radius;
        int dz = rand() % (radius * 2 + 1) - radius;
        int dy = rand() % 3 - 1;

        glm::ivec3 pos = start + glm::ivec3(dx, dy, dz);
        glm::ivec3 below(pos.x, pos.y - 1, pos.z);

        // Ensure that the node is somewhere he can stand, not just reach once by jumping.
        if (!pathfinder.is_walkable(pos, 1) || pathfinder.is_walkable(below, 1))
            continue;

        float dist = glm::distance(glm::vec3(pos), glm::vec3(threat));

        float noise = (float)std::rand() / RAND_MAX * 2.0f - 1.0f;

        float score = dist + noise;

        // The more the node is far, the more he has better score.
        if (score > best_score)
        {
            best_score = score;
            best = pos;
        }
    }

    return best;
}

void Cow::flee_from(const Entity& threat, int radius)
{
    glm::ivec3 cow_grid = glm::ivec3(glm::round(m_transform.position()));
    glm::ivec3 threat_grid = glm::ivec3(glm::round(threat.get_global_transform().position()));

    glm::ivec3 target = find_flee_target(*m_pathfinding, cow_grid, threat_grid, radius);

    m_pathfinding->find_path(cow_grid, target);
    println("Cow fleeing to [{} {} {}]", target.x, target.y, target.z);
}