#pragma once
#include "base.hpp"

namespace zephyr {

struct EntityUniformBuffer {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
};

} // namespace zephyr
