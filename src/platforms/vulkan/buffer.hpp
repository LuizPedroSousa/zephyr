#pragma once

#include "assert.hpp"
#include "platforms/vulkan/command-buffer.hpp"
#include "platforms/vulkan/device.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

namespace VulkanBuffer {
static VkBufferCopy declare_copy_region(VkDeviceSize source_offset,
                                        VkDeviceSize destination_offset,
                                        VkDeviceSize size) {
  VkBufferCopy copy_region{};
  copy_region.srcOffset = source_offset;
  copy_region.dstOffset = destination_offset;
  copy_region.size = size;

  return copy_region;
}

template <typename T>
T static make(VulkanLogicalDevice logical_device, VkDeviceSize size,
              VkBufferUsageFlags usage) {
  VkBufferCreateInfo buffer_info{};

  T region;
  region.size = size;
  region.offset = 0;
  region.ld_handle = logical_device.handle;

  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  ZEPH_ENSURE(vkCreateBuffer(logical_device.handle, &buffer_info, nullptr,
                             &region.buffer) != VK_SUCCESS,
              "Couldn't create buffer")

  return region;
}

template <typename T>
void allocate(T region, VulkanPhysicalDevice physical_device,
              VkMemoryPropertyFlags properties) {
  VkMemoryRequirements mem_requirements;

  vkGetBufferMemoryRequirements(region->ld_handle, region->buffer,
                                &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = VulkanPhysicalDevice::find_memory_type(
      physical_device.handle, mem_requirements.memoryTypeBits, properties);

  ZEPH_ENSURE(vkAllocateMemory(region->ld_handle, &alloc_info, nullptr,
                               &region->memory) != VK_SUCCESS,
              "Couldn't allocate vertex buffer memory");

  vkBindBufferMemory(region->ld_handle, region->buffer, region->memory, 0);
}

template <typename T> void map(T region) {
  vkMapMemory(region->ld_handle, region->memory, 0, (size_t)region->size, 0,
              &region->mapped);
}

template <typename T> void upload(T region, void *data) {
  map(region);
  memcpy(region->mapped, data, region->size);
}

template <typename S, typename D>
void copy(S source_region, D destination_region, VkDeviceSize size,
          VkQueue queue, VkCommandPool command_pool) {

  ZEPH_ENSURE(source_region->ld_handle != destination_region->ld_handle,
              "Can't handle cross device copy");

  VkCommandBuffer command_buffer;

  VkCommandBufferAllocateInfo allocate_info =
      VulkanCommandBuffer::declare_allocate(1, command_pool);

  ZEPH_ENSURE(vkAllocateCommandBuffers(source_region->ld_handle, &allocate_info,
                                       &command_buffer) != VK_SUCCESS,
              "Couldn't allocate command buffer");

  VkCommandBufferBeginInfo begin_info = VulkanCommandBuffer::declare_begin(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region = VulkanBuffer::declare_copy_region(0, 0, size);

  vkCmdCopyBuffer(command_buffer, source_region->buffer,
                  destination_region->buffer, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  std::vector<VkCommandBuffer> command_buffers = {command_buffer};
  VkSubmitInfo submit_info = VulkanQueue::declare_submit(command_buffers);

  vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(source_region->ld_handle, command_pool, 1,
                       &command_buffer);
}

struct TransientStagingRegion {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;
  VkDeviceSize offset = 0;
  VkDeviceSize size = 0;
  void *mapped = nullptr;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  static TransientStagingRegion make(VulkanLogicalDevice logical_device,
                                     VkDeviceSize size,
                                     VkBufferUsageFlags usage) {
    return VulkanBuffer::make<TransientStagingRegion>(logical_device, size,
                                                      usage);
  }

  void allocate(VulkanPhysicalDevice physical_device,
                VkMemoryPropertyFlags properties) {

    VulkanBuffer::allocate(this, physical_device, properties);
  }

  void map() { VulkanBuffer::map(this); }

  void unmap() {
    if (mapped) {
      vkUnmapMemory(ld_handle, memory);
    }
  }

  void upload(void *data) { VulkanBuffer::upload(this, data); }

  void cleanup() {
    vkDestroyBuffer(ld_handle, buffer, nullptr);
    vkFreeMemory(ld_handle, memory, nullptr);
  }

  void flush(VkDeviceSize flush_offset,
             VkDeviceSize flush_size = VK_WHOLE_SIZE) {
    if (mapped) {
      VkMappedMemoryRange range{};
      range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range.memory = memory;
      range.offset = flush_offset;
      range.size = flush_size;
      vkFlushMappedMemoryRanges(ld_handle, 1, &range);
    }
  }
};

struct DeviceLocalRegion {
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;
  VkDeviceSize offset = 0;
  VkDeviceSize size = 0;
  void *mapped = nullptr;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  static DeviceLocalRegion make(VulkanLogicalDevice logical_device,
                                VkDeviceSize size, VkBufferUsageFlags usage) {
    return VulkanBuffer::make<DeviceLocalRegion>(logical_device, size, usage);
  }

  void allocate(VulkanPhysicalDevice physical_device,
                VkMemoryPropertyFlags properties) {

    VulkanBuffer::allocate(this, physical_device, properties);
  }

  void map() { VulkanBuffer::map(this); }

  void unmap(VulkanLogicalDevice logical_device, VkDeviceMemory memory_buffer) {
    if (mapped) {
      vkUnmapMemory(logical_device.handle, memory_buffer);
    }
  }

  void upload(void *data) { VulkanBuffer::upload(this, data); }

  template <typename T>
  void copy_from(T *source_region, VkQueue queue, VkCommandPool command_pool) {
    VulkanBuffer::copy(source_region, this, size, queue, command_pool);
  }

  void cleanup() {
    vkDestroyBuffer(ld_handle, buffer, nullptr);
    vkFreeMemory(ld_handle, memory, nullptr);
  }

  void flush(VkDeviceSize flush_offset,
             VkDeviceSize flush_size = VK_WHOLE_SIZE) {
    if (mapped) {
      VkMappedMemoryRange range{};
      range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range.memory = memory;
      range.offset = flush_offset;
      range.size = flush_size;
      vkFlushMappedMemoryRanges(ld_handle, 1, &range);
    }
  }
};

}; // namespace VulkanBuffer

} // namespace zephyr
