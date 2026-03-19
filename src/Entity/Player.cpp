#include "Entity/Player.hpp"
#include "AABB.hpp"
#include "Core/Definitions.hpp"
#include "Entity/Entity.hpp"
#include "Input.hpp"
#include "Profiler.hpp"
#include "World/Dimension.hpp"
#include "World/Registry.hpp"
#include "glm/gtc/epsilon.hpp"

#include <cmath>

static float line_to_plane(glm::vec3 p, glm::vec3 u, glm::vec3 v, glm::vec3 n)
{
    const float n_dot_u = n.x * u.x + n.y * u.y + n.z * u.z;
    if (n_dot_u == 0)
        return INFINITY;
    return (n.x * (v.x - p.x) + n.y * (v.y - p.y) + n.z * (v.z - p.z)) / n_dot_u;
}

static ALWAYS_INLINE bool between(float x, float a, float b)
{
    return x >= a && x <= b;
}

struct CollisionResult sweep_aabb(const AABB& a, const AABB& b, glm::vec3 d)
{
    const float mx = b.center.x - (a.center.x + a.half_extent.x);
    const float my = b.center.y - (a.center.y + a.half_extent.y);
    const float mz = b.center.z - (a.center.z + a.half_extent.z);
    const float mhx = a.half_extent.x + b.half_extent.x;
    const float mhy = a.half_extent.y + b.half_extent.y;
    const float mhz = a.half_extent.z + b.half_extent.z;
    CollisionResult r(glm::vec3(), 1.0);
    float s;

    // x min
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx, my, mz), glm::vec3(-1, 0, 0));
    if (s >= 0 && d.x > 0 && s < r.penetration && between(s * d.y, my, my + mhy) && between(s * d.z, mz, mz + mhz))
    {
        r.penetration = s;
        r.normal = glm::vec3(-1, 0, 0);
    }
    // x max
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx + mhx, my, mz), glm::vec3(1, 0, 0));
    if (s >= 0 && d.x < 0 && s < r.penetration && between(s * d.y, my, my + mhy) && between(s * d.z, mz, mz + mhz))
    {
        r.penetration = s;
        r.normal = glm::vec3(1, 0, 0);
    }

    // y min
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx, my, mz), glm::vec3(0, -1, 0));
    if (s >= 0 && d.y > 0 && s < r.penetration && between(s * d.x, mx, mx + mhx) && between(s * d.z, mz, mz + mhz))
    {
        r.penetration = s;
        r.normal = glm::vec3(0, -1, 0);
    }
    // y max
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx, my + mhy, mz), glm::vec3(0, 1, 0));
    if (s >= 0 && d.y < 0 && s < r.penetration && between(s * d.x, mx, mx + mhx) && between(s * d.z, mz, mz + mhz))
    {
        r.penetration = s;
        r.normal = glm::vec3(0, 1, 0);
    }

    // z min
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx, my, mz), glm::vec3(0, 0, -1));
    if (s >= 0 && d.z > 0 && s < r.penetration && between(s * d.x, mx, mx + mhx) && between(s * d.y, my, my + mhy))
    {
        r.penetration = s;
        r.normal = glm::vec3(0, 0, -1);
    }
    // z max
    s = line_to_plane(glm::vec3(), d, glm::vec3(mx, my, mz + mhz), glm::vec3(0, 0, 1));
    if (s >= 0 && d.z < 0 && s < r.penetration && between(s * d.x, mx, mx + mhx) && between(s * d.y, my, my + mhy))
    {
        r.penetration = s;
        r.normal = glm::vec3(0, 0, 1);
    }

    return r;
}

void Player::move_and_collide()
{
    // TODO: high velocity might miss some collisions. Probably should decompose collisions in multiple steps if that the case
    const AABB pbox(m_transform.position(), glm::vec3(0.35, 0.9, 0.35));
    const Dimension& dimension = m_world->get_dimension(m_dimension);
    const std::vector<AABB> colliders = dimension.get_boxes_that_may_collide(pbox);

    constexpr size_t max_iteration = 1;
    size_t iteration = 0;
    while (iteration < max_iteration)
    {
        CollisionResult closest_collision(glm::vec3(), 1.0);

        for (const auto& block : colliders)
        {
            const CollisionResult collision = sweep_aabb(pbox, block, m_velocity);
            if (collision.penetration < closest_collision.penetration)
                closest_collision = collision;
        }

        const float margin = 0.001;
        m_transform.position() += closest_collision.penetration * m_velocity + margin * closest_collision.normal;

        // TODO: what if its not possible to resolve a collision ? it should not infinite loop
        if (closest_collision.penetration == 1.0)
            break;

        // const float b_dot_b = glm::dot(closest_collision.normal, closest_collision.normal);
        // if (glm::epsilonEqual(b_dot_b, 0.0f, 0.0001f))
        // {
        //     const float one_minus_p = 1.0f - closest_collision.penetration;
        //     const float a_dot_b = one_minus_p * glm::dot(m_velocity, closest_collision.normal);
        //     m_transform.position() += one_minus_p * m_velocity - (a_dot_b / b_dot_b) * closest_collision.normal;
        // }

        iteration++;
    }
}

void Player::tick(float delta)
{
    ZoneScopedN("Player::tick");

    (void)delta;

    if (Input::is_action_pressed("attack") && !Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(true);
        return;
    }
    else if (Input::is_action_pressed("escape") && Input::is_mouse_grabbed())
    {
        Input::set_mouse_grabbed(false);
        return;
    }

    Transform3D transform = m_transform;

    const glm::vec3 up(0.0, 1.0, 0.0);

    if (Input::is_mouse_grabbed())
    {
        ZoneScopedN("Player::tick.mouse_movements");

        constexpr float mouse_sensibility = 0.03f;

        const glm::vec2 relative = Input::get_mouse_relative();
        const glm::quat q_yaw = glm::angleAxis(relative.x * mouse_sensibility, up);

        transform.rotation() *= q_yaw;
        m_transform = transform;

        Ref<Camera> camera = m_children[0].cast_to<Camera>();
        Transform3D camera_transform = camera->get_transform();

        const glm::quat q_pitch = glm::angleAxis(relative.y * mouse_sensibility, glm::vec3(1.0, 0.0, 0.0));
        camera_transform.rotation() *= q_pitch;

        camera->get_transform() = camera_transform;
    }

    // {
    //     ZoneScopedN("Player::tick.aim");

    //     const glm::vec3 camera_forward = result.children<Transformed3D, Camera>()[0].get<Transformed3D>()->get_global_transform().forward();
    //     Ray ray(transform.position(), camera_forward);
    //     float t = 0.0;
    //     while (t < 5.0)
    //     {
    //         glm::vec3 pos = ray.at(t);

    //         int64_t x = (int64_t)(pos.x + 0.5);
    //         int64_t y = (int64_t)(pos.y + 0.5);
    //         int64_t z = (int64_t)(pos.z + 0.5);

    //         BlockState state = m_world->get_block_state(x, y, z);

    //         if (!state.is_air())
    //         {
    //             // m_cube_highlight->get_component<MeshInstance>()->set_visible(true);
    //             on_block_aimed(state, x, y, z, ray.dir());
    //             break;
    //         }
    //         else
    //         {
    //             // m_cube_highlight->get_component<MeshInstance>()->set_visible(false);
    //         }

    //         t += 0.1;
    //     }
    // }

    const glm::vec3 forward = get_global_transform().forward();
    const glm::vec3 right = get_global_transform().right();

    const glm::vec2 dir = Input::get_vector("left", "right", "backward", "forward");
    const float updown_dir = Input::get_action_value("up") - Input::get_action_value("down");

    // println("[ {} {} ]", dir.x, dir.y);

    if (glm::length2(dir) != 0.0 || updown_dir != 0.0)
    {
        glm::vec3 move = glm::normalize(forward * dir.y + right * dir.x + up * updown_dir) * m_speed;
        m_velocity += move * delta;
        move_and_collide();
    }

    // Reset velocity after movements.
    m_velocity = glm::vec3(0);

    // move_and_collide(transform_comp, body, player, world);

    // if (!m_gravity_enabled)
    // {
    //     velocity += up * updown_dir * m_speed;
    // }

    // if (m_body->is_on_ground(m_world) && Input::is_action_pressed("up") && !m_has_jumped && m_gravity_enabled)
    // {
    //     m_body->velocity() += up * 0.1f;
    //     m_has_jumped = true;
    // }
    // else if (!Input::is_action_pressed("up"))
    // {
    //     m_has_jumped = false;
    // }

    // if (m_gravity_enabled && !m_body->is_on_ground(m_world))
    // {
    //     m_body->velocity().y -= m_gravity_value * (float)delta;
    // }

    // m_body->move_and_collide(m_world, delta);

    // m_body->velocity().x = 0; // FIXME: Add friction, ...
    // m_body->velocity().z = 0;

    // if (!m_gravity_enabled)
    // {
    //     m_body->velocity().y = 0;
    // }
}

enum class Face
{
    North,
    South,
    West,
    East,
    Top,
    Bottom,
};

static Face get_face(glm::vec3 dir)
{
    std::array<glm::vec3, 6> normals{
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };
    std::array<Face, 6> faces{Face::North, Face::South, Face::West, Face::East, Face::Top, Face::Bottom};

    float max_dot = 0.0;
    Face max_face = Face::North;

    for (size_t i = 0; i < 6; i++)
    {
        glm::vec3 normal = -normals[i];
        Face face = faces[i];

        float dot = glm::dot(dir, normal);

        if (dot >= 0.0 && dot > max_dot)
        {
            max_dot = dot;
            max_face = face;
        }
    }

    return max_face;
}

void Player::on_block_aimed(BlockState state, int64_t x, int64_t y, int64_t z, glm::vec3 dir)
{
    Transform3D transform(glm::vec3((float)x, (float)y, (float)z));
    // FIXME: transform does not matter for MeshInstance, probably should instead of manipulating a buffer directly.
    // m_cube_highlight->get_transform->set_transform(transform);

    // CubeHighlightUniforms uniforms{};
    // uniforms.model_matrix = transform.translation_matrix();
    // uniforms.color = glm::vec3(1.0, 1.0, 0.0);

    // RenderingDriver::get()->update_buffer(m_cube_highlight_buffer, View(uniforms).as_bytes(), 0);

    if (Input::is_action_pressed("attack") && !state.is_air())
    {
        m_world->set_block_state(x, y, z, BlockState());
    }
    else if (Input::is_action_pressed("place"))
    {
        Face face = get_face(dir);

        int64_t x2 = face == Face::West ? x + 1 : (face == Face::East ? x - 1 : x);
        int64_t y2 = face == Face::Top ? y + 1 : (face == Face::Bottom ? y - 1 : y);
        int64_t z2 = face == Face::South ? z + 1 : (face == Face::North ? z - 1 : z);

        m_world->set_block_state(x2, y2, z2, BlockState(BlockRegistry::get_block_id("stone")));
    }
}
