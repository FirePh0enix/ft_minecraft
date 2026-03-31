#include "Cow.hpp"
#include "Core/Print.hpp"
#include "Entity/Entity.hpp"
#include "Render/Driver.hpp"
#include "World/World.hpp"
#include "glm/ext/vector_float3.hpp"

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

    if (m_pathfinding->m_path_index < m_pathfinding->m_path.size())
    {

        Node *next_node = m_pathfinding->m_path[m_pathfinding->m_path_index];
        glm::vec3 cow_pos = get_global_transform().position();
        glm::vec3 dir = next_node->m_position - cow_pos;
        float dist = glm::length(dir);

        if (dist <= 0.1f)
        {
            m_pathfinding->m_path_index++;
            println("Cow::tick() path index: {}, total index: {}", m_pathfinding->m_path_index, m_pathfinding->m_path.size());
        }
        else
        {
            dir = glm::normalize(dir);
            float speed = 2.0f;
            m_velocity = dir * speed * delta;
        }
    }
    else
        m_velocity = glm::vec3(0);

    move_and_collide(true);
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
}

void Cow::on_hit_by(Entity& entity)
{
    Mob *mob_caller = dynamic_cast<Mob *>(&entity);
    if (!mob_caller)
        return;

    int damage = mob_caller->get_attack_damage();
    take_damage(damage);
    println("Hit by {} Damage: {}", entity.get_name(), damage);

    if (m_health <= 0)
    {
        die();
        return;
    }

    const glm::vec3 cow_pos = get_global_transform().position();

    const glm::vec3 target_pos = cow_pos + glm::vec3(0, 0, -10);

    m_pathfinding->m_path_index = 0;
    m_pathfinding->find_path(cow_pos, target_pos);
}