#include "Scene/Components/RigidBody.hpp"

void RigidBody::start()
{
}

void RigidBody::tick(double delta)
{
}

bool RigidBody::intersect_world(Ref<World> world)
{
    std::array<glm::vec3, 8> corners;

    int64_t x = (int64_t)corners[0].x;
    int64_t y = (int64_t)corners[0].y;
    int64_t z = (int64_t)corners[0].z;

    BlockState state = world->get_block_state(x, y, z);

    if (!state.is_air())
    {
        return true;
    }

    return false;
}
