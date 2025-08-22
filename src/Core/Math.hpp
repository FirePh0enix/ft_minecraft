#pragma once

namespace math
{

inline bool is_zero_e(float f)
{
    return glm::abs(f) < glm::epsilon<float>();
}

inline bool is_zero_e(const glm::vec3& vec)
{
    return glm::abs(glm::length(vec)) < glm::epsilon<float>();
}

} // namespace math
