

#pragma once

#include "platforms/vulkan/command-buffer.hpp"
#include "platforms/vulkan/buffer.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

void VulkanCommandBuffer::copy(VkBuffer from, VkBuffer to, VkDeviceSize size) {
  VkBufferCopy copy_region = VulkanBuffer::declare_copy_region(0, 0, size);
  vkCmdCopyBuffer(handle, from, to, 1, &copy_region);
}

} // namespace zephyr
//
