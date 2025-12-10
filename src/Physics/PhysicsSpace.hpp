#pragma once

#include "Physics/PhysicsBody.hpp"
#include "Scene/System.hpp"

#include <algorithm>

class PhysicsSpace
{
public:
    PhysicsSpace();

    void step(float delta);

    void add_body(PhysicsBody *body)
    {
        if (body->get_kind() == PhysicsBodyKind::Kinematic || body->get_kind() == PhysicsBodyKind::Rigid)
            m_dynamic_bodies.push_back(body);
        else if (body->get_kind() == PhysicsBodyKind::Static)
            m_static_bodies.push_back(body);
    }

    void remove_body(PhysicsBody *body)
    {
        auto iter = std::find(m_dynamic_bodies.begin(), m_dynamic_bodies.end(), body);
        if (iter != m_dynamic_bodies.end())
        {
            m_dynamic_bodies.erase(iter);
            return;
        }

        auto iter2 = std::find(m_static_bodies.begin(), m_static_bodies.end(), body);
        if (iter2 != m_static_bodies.end())
        {
            m_static_bodies.erase(iter2);
            return;
        }
    }

    CollisionResult test_collision(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b);
    void resolve_collisions(float delta);

private:
    std::vector<PhysicsBody *> m_dynamic_bodies;
    std::vector<PhysicsBody *> m_static_bodies;
};

class Scene;
class RigidBody;
class Transformed3D;

struct PhysicsPlugin
{
    static void setup(Scene *scene);
    static void sync_rigidbody_transform_system(const Query<Many<Transformed3D, RigidBody>>& query, Action&);
};
