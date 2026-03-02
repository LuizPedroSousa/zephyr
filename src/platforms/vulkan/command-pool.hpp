#pragma once

#include "assert.hpp"
#include "platforms/vulkan/command-buffer.hpp"
#include "platforms/vulkan/device.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanCommandPool {
public:
  VkCommandPool handle = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> commands;

  static VkCommandPoolCreateInfo declare(uint32_t queue_family_index) {
    VkCommandPoolCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue_family_index;

    return create_info;
  }

  static VulkanCommandPool make(VulkanLogicalDevice ld,
                                VulkanPhysicalDevice ph) {
    VulkanCommandPool pool;
    pool.ld_handle = ld.handle;

    auto create_info = declare(ph.queue_family_indices.graphics_family.value());

    ZEPH_ENSURE(vkCreateCommandPool(ld.handle, &create_info, nullptr,
                                    &pool.handle) != VK_SUCCESS,
                "Couldn't create command pool");

    return pool;
  };

  void allocate(size_t command_size) {
    commands.resize(command_size);

    VkCommandBufferAllocateInfo alloc_info =
        VulkanCommandBuffer::declare_allocate(command_size, handle);

    ZEPH_ENSURE(vkAllocateCommandBuffers(ld_handle, &alloc_info,
                                         commands.data()) != VK_SUCCESS,
                "Couldn't allocate command buffers on pool");
  }

  void allocate(VulkanCommandBuffer &command) {
    VkCommandBufferAllocateInfo alloc_info =
        VulkanCommandBuffer::declare_allocate(1, handle);

    ZEPH_ENSURE(vkAllocateCommandBuffers(ld_handle, &alloc_info,
                                         &command.handle) != VK_SUCCESS,
                "Couldn't allocate command buffers on pool");
  }

  void flush(VulkanCommandBuffer &command) {
    vkFreeCommandBuffers(ld_handle, handle, 1, &command.handle);
  }

  void push_command_buffer(VkCommandBuffer command_buffer) {
    commands.push_back(command_buffer);
  }

  void cleanup() { vkDestroyCommandPool(ld_handle, handle, nullptr); }
};

} // namespace zephyr
