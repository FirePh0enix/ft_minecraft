#pragma once

// #define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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
