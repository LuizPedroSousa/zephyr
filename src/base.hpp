#pragma once

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

#include <stdexcept>

namespace zephyr {

using VertexIndice = uint32_t;

} // namespace zephyr
