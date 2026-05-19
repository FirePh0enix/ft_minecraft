#pragma once

// #define GLM_FORCE_LEFT_HANDED
#include <cstdlib>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <SDL3/SDL.h>

namespace math
{

inline bool is_zero_e(float f)
{
    return glm::abs(f) <= glm::epsilon<float>();
}

inline bool is_zero_e(const glm::vec3& vec)
{
    return glm::length2(vec) <= glm::epsilon<float>() * glm::epsilon<float>();
}

} // namespace math

inline float rand_float(float min, float max)
{
    float midpoint = max / 2 + min / 2;
    float half_range = max / 2 - min / 2;
    int plus_minus = (rand() & 1) == 1 ? 1 : -1;
    return midpoint + float(plus_minus) * (float(rand()) / float(RAND_MAX)) * half_range;
}
