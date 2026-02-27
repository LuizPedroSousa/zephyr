#pragma once

#include <vulkan/vulkan_core.h>

namespace zephyr {
class VulkanSemaphore {
public:
  static VkSemaphoreCreateInfo declare() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    return semaphore_info;
  }
};

} // namespace zephyr
