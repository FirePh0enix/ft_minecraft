#include "Physics/PhysicsSpace.hpp"
#include "Scene/Components/RigidBody.hpp"
#include "Scene/Components/Transformed3D.hpp"
#include "Scene/Scene.hpp"

static CollisionResult test_box_box(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b);
static CollisionResult test_box_grid(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b);

static TestCollisionFunc test_collision_funcs[2][2]{
    {test_box_box, test_box_grid}, // Box
    {nullptr, test_box_grid},      // Grid
};

static CollisionResult test_box_box(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b)
{
    (void)collider_a;
    (void)collider_b;
    (void)position_a;
    (void)position_b;
    return CollisionResult();
}

static CollisionResult test_box_grid(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b)
{
    (void)position_a;
    (void)position_b;

    BoxCollider *box = static_cast<BoxCollider *>(collider_a);
    GridCollider *grid = static_cast<GridCollider *>(collider_b);

    AABB grid_aabb(position_b, glm::vec3(grid->width, grid->height, grid->depth) * grid->voxel_size);

    if (grid_aabb.intersect(box->aabb))
        return CollisionResult();

    for (size_t x = 0; x < grid->width; x++)
    {
        for (size_t y = 0; y < grid->height; y++)
        {
            for (size_t z = 0; z < grid->depth; z++)
            {
                if (!grid->has(x, y, z))
                    continue;

                glm::vec3 voxel_min = position_b + glm::vec3(x, y, z) * grid->voxel_size;
                AABB voxel_box = AABB(voxel_min + grid->voxel_size / 2.0f, grid->voxel_size);

                // if (box->aabb.intersect(voxel_box))
                //     println("INTERSECTION");
            }
        }
    }

    return CollisionResult();
}

PhysicsSpace::PhysicsSpace()
{
}

void PhysicsSpace::step(float delta)
{
    for (PhysicsBody *body : m_dynamic_bodies)
        body->step(delta);

    resolve_collisions(delta);
}

CollisionResult PhysicsSpace::test_collision(Collider *collider_a, glm::vec3 position_a, Collider *collider_b, glm::vec3 position_b)
{
    bool swap = false;

    if (collider_a->type > collider_b->type)
    {
        std::swap(collider_a, collider_b);
        swap = true;
    }

    CollisionResult result = CollisionResult();
    TestCollisionFunc func = test_collision_funcs[(int)collider_a->type][(int)collider_b->type];
    if (func)
        result = func(collider_a, position_a, collider_b, position_b);

    if (swap)
    {
        std::swap(result.a, result.b);
        result.normal *= -1;
    }

    return result;
}

void PhysicsSpace::resolve_collisions(float delta)
{
    (void)delta;
    std::vector<CollisionResult> collisions;

    for (PhysicsBody *body_a : m_dynamic_bodies)
    {
        for (PhysicsBody *body_b : m_static_bodies)
        {
            if (!body_a->get_collider() || !body_b->get_collider())
                continue;

            CollisionResult result = test_collision(body_a->get_collider(), body_a->get_position(), body_b->get_collider(), body_b->get_position());

            if (result.has_collision)
                collisions.push_back(result);
        }
    }

    for (PhysicsBody *body_a : m_dynamic_bodies)
    {
        for (PhysicsBody *body_b : m_dynamic_bodies)
        {
            if (body_a == body_b)
                break;

            if (!body_a->get_collider() || !body_b->get_collider())
                continue;

            CollisionResult result = test_collision(body_a->get_collider(), body_a->get_position(), body_b->get_collider(), body_b->get_position());

            if (result.has_collision)
                collisions.push_back(result);
        }
    }

    for (const CollisionResult& collision : collisions)
    {
        (void)collision;
    }
}

void PhysicsPlugin::setup(Scene *scene)
{
    scene->add_system(EarlyFixedUpdate, sync_rigidbody_transform_system);
}

void PhysicsPlugin::sync_rigidbody_transform_system(const Query<Many<Transformed3D, RigidBody>>& query, Action&)
{
    for (const auto& result : query.get<0>().results())
        result.get<Transformed3D>()->get_transform().position() = result.get<RigidBody>()->get_body()->get_position();
}
