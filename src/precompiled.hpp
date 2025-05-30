#ifdef __USE_VULKAN__

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#define VULKAN_HPP_ASSERT_ON_RESULT(X)

#ifdef __TARGET_APPLE__
#define VK_ENABLE_BETA_EXTENSIONS
#endif

#include <vulkan/vulkan.hpp>

#endif

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

#include <tracy/Tracy.hpp>

#ifdef __USE_VULKAN__
#define TRACY_VK_USE_SYMBOL_TABLE

#include <tracy/TracyVulkan.hpp>
#endif
