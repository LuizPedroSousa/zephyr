#pragma once

#include "assert.hpp"
#include "platforms/vulkan/device.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanCommandBuffer {
public:
  VkCommandBuffer handle = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;

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

  static VulkanCommandBuffer make(VkDevice ld_handle) {
    VulkanCommandBuffer command_buffer;
    command_buffer.ld_handle = ld_handle;

    return command_buffer;
  };

  static void begin_command_buffer(VkCommandBuffer command_buffer) {
    VkCommandBufferBeginInfo begin_info = VulkanCommandBuffer::declare_begin(0);

    ZEPH_ENSURE(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS,
                "Couldn't begin to push command buffer");
  }

  static void end_command_buffer(VkCommandBuffer command_buffer) {
    ZEPH_ENSURE(vkEndCommandBuffer(command_buffer) != VK_SUCCESS,
                "Couldn't create shader");
  }

  void begin(VkCommandBufferUsageFlags usage =
                 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
    VkCommandBufferBeginInfo begin_info =
        VulkanCommandBuffer::declare_begin(usage);
    vkBeginCommandBuffer(handle, &begin_info);
  }

  void copy(VkBuffer from, VkBuffer to, VkDeviceSize size);

  void copy_to_image(VkBuffer buffer, VkImage image,
                     VkBufferImageCopy &region) {
    vkCmdCopyBufferToImage(handle, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  }

  void pipeline_barrier(VkPipelineStageFlags src_stage,
                        VkPipelineStageFlags dst_stage,
                        VkImageMemoryBarrier &barrier) {
    vkCmdPipelineBarrier(handle, src_stage, dst_stage, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
  };

  void end() { vkEndCommandBuffer(handle); }
};

} // namespace zephyr
