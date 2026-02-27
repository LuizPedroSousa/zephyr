#pragma once

#include "memory"
#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/ext/scalar_uint_sized.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#define BACKEND_VULKAN 1
#define BACKEND_OPENGL 2

namespace zephyr {

using VertexIndice = uint32_t;

template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr Scope<T> create_scope(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
};

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T, typename... Args>
constexpr Ref<T> create_ref(Args &&...args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
};

} // namespace zephyr
