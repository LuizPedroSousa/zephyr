#pragma once

#include <cstdint>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define BACKEND_VULKAN 1
#define BACKEND_OPENGL 2

#include <stdexcept>

namespace zephyr {

using VertexIndice = uint32_t;

} // namespace zephyr
