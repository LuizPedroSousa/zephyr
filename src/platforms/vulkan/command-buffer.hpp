#pragma once

#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanCommandBuffer {
public:
  static VkCommandBufferAllocateInfo
  declare_allocate(uint32_t command_buffer_count, VkCommandPool command_pool) {
    VkCommandBufferAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = command_buffer_count;

    return allocate_info;
  }

  VkCommandBufferBeginInfo static declare_begin(
      VkCommandBufferUsageFlags usage) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = usage;

    return begin_info;
  }
};

} // namespace zephyr
