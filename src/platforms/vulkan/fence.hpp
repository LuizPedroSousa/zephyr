#pragma once

#include <vulkan/vulkan_core.h>

namespace zephyr {
class VulkanFence {
public:
  static VkFenceCreateInfo
  declare(VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT) {
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = flags;

    return fence_info;
  }
};

} // namespace zephyr
