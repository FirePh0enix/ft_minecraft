#pragma once

#include "Physics/PhysicsBody.hpp"
#include "Scene/System.hpp"

#include <algorithm>

class PhysicsSpace
{
public:
    PhysicsSpace();

    void step(float delta);

    void add_body(PhysicsBody *body) { m_bodies.push_back(body); }
    void remove_body(PhysicsBody *body) { m_bodies.erase(std::find(m_bodies.begin(), m_bodies.end(), body)); }

    CollisionResult test_collision(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b);
    void resolve_collisions(float delta);

private:
    std::vector<PhysicsBody *> m_bodies;
};

class Scene;
class RigidBody;
class Transformed3D;

struct PhysicsPlugin
{
    static void setup(Scene *scene);
    static void sync_rigidbody_transform_system(const Query<Many<Transformed3D, RigidBody>>& query, Action&);
};
