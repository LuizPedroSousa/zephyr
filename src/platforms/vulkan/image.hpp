#pragma once

#include "assert.hpp"
#include "platforms/vulkan/buffer.hpp"
#include "platforms/vulkan/command-buffer.hpp"
#include "platforms/vulkan/command-pool.hpp"
#include "platforms/vulkan/device.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

namespace VulkanBuffer {

struct VulkanImageView {
  VkImageView handle = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;

  static VulkanImageView make(VkDevice ld_handle, VkImage image,
                              VkFormat format) {
    VulkanImageView view;
    view.ld_handle = ld_handle;

    VkImageViewCreateInfo view_info{};

    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;

    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    ZEPH_ENSURE(vkCreateImageView(ld_handle, &view_info, nullptr,
                                  &view.handle) != VK_SUCCESS,
                "Couldn't create image view");

    return view;
  }

  void cleanup() { vkDestroyImageView(ld_handle, handle, nullptr); }
};

struct VulkanSampler {
  VkSampler handle = VK_NULL_HANDLE;
  VkDevice ld_handle = VK_NULL_HANDLE;

  static VulkanSampler make(VkDevice ld_handle, VkPhysicalDevice ph_handle) {
    VulkanSampler s;
    s.ld_handle = ld_handle;

    VkSamplerCreateInfo sampler_info{};

    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;

    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};

    vkGetPhysicalDeviceProperties(ph_handle, &properties);

    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    ZEPH_ENSURE(vkCreateSampler(ld_handle, &sampler_info, nullptr, &s.handle) !=
                    VK_SUCCESS,
                "Couldn't create sampler");

    return s;
  }

  void cleanup() { vkDestroySampler(ld_handle, handle, nullptr); }
};

class VulkanImageRegion {
public:
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VulkanImageView image_view;
  VulkanSampler sampler;

  VkDevice ld_handle = VK_NULL_HANDLE;
  VkPhysicalDevice ph_handle = VK_NULL_HANDLE;

  uint32_t width = 0;
  uint32_t height = 0;
  VkFormat format;
  VkImageTiling tiling;

  VkImageLayout layout;
  VkImageLayout previous_layout;

  static VkImageCreateInfo declare(VkImageUsageFlags usage, uint32_t width,
                                   uint32_t height, VkFormat format,
                                   VkImageTiling tiling, VkImageLayout layout) {
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = static_cast<uint32_t>(width);
    image_info.extent.height = static_cast<uint32_t>(height);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = layout;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    return image_info;
  }

  static VulkanImageRegion make(VulkanLogicalDevice ld, uint32_t width,
                                uint32_t height, VkFormat format,
                                VkImageTiling tiling, VkImageUsageFlags usage) {
    VulkanImageRegion region;
    region.width = width;
    region.height = height;
    region.tiling = tiling;
    region.format = format;
    region.ld_handle = ld.handle;
    region.ph_handle = ld.phy_handle;
    region.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo image_info =
        declare(usage, width, height, format, tiling, region.layout);

    ZEPH_ENSURE(vkCreateImage(ld.handle, &image_info, nullptr, &region.image) !=
                    VK_SUCCESS,
                "Couldn't create texture image");

    return region;
  }

  void allocate(VulkanPhysicalDevice physical_device,
                VkMemoryPropertyFlags device_properties) {
    VkMemoryRequirements mem_requirements;

    vkGetImageMemoryRequirements(ld_handle, image, &mem_requirements);

    VkMemoryAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = mem_requirements.size;
    allocate_info.memoryTypeIndex = VulkanPhysicalDevice::find_memory_type(
        physical_device.handle, mem_requirements.memoryTypeBits,
        device_properties);

    ZEPH_ENSURE(vkAllocateMemory(ld_handle, &allocate_info, nullptr, &memory) !=
                    VK_SUCCESS,
                "Couldn't allocate image memory");

    vkBindImageMemory(ld_handle, image, memory, 0);

    image_view = VulkanImageView::make(ld_handle, image, format);
    sampler = VulkanSampler::make(ld_handle, ph_handle);
  }

  void transition_layout(VkQueue queue, VulkanCommandPool command_pool,
                         VkImageLayout new_layout) {
    auto command_buffer = VulkanCommandBuffer::make(command_pool.ld_handle);

    command_pool.allocate(command_buffer);

    command_buffer.begin();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      ZEPH_EXCEPTION("Unsupported layout transition");
    }

    previous_layout = layout;
    layout = new_layout;

    command_buffer.pipeline_barrier(src_stage, dst_stage, barrier);

    command_buffer.end();

    VulkanQueue::submit(queue, command_buffer.handle, true);
    command_pool.flush(command_buffer);
  }

  void copy_to_image(VulkanBuffer::TransientStagingRegion staging_buffer,
                     VkQueue queue, VulkanCommandPool command_pool) {
    ZEPH_ENSURE(layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                "Couldn't copy buffer to image, the layout must be set first "
                "to optimal transfer destination.");

    auto command_buffer = VulkanCommandBuffer::make(ld_handle);

    command_pool.allocate(command_buffer);

    command_buffer.begin();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    command_buffer.copy_to_image(staging_buffer.buffer, image, region);

    command_buffer.end();

    VulkanQueue::submit(queue, command_buffer.handle, true);
    command_pool.flush(command_buffer);
  }

  void cleanup() {
    sampler.cleanup();
    image_view.cleanup();
    vkDestroyImage(ld_handle, image, nullptr);
    vkFreeMemory(ld_handle, memory, nullptr);
  }
};

}; // namespace VulkanBuffer

} // namespace zephyr
