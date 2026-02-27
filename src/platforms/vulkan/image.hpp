#pragma once

#include "assert.hpp"
#include "platforms/vulkan/device.hpp"
#include <vulkan/vulkan_core.h>

namespace zephyr {

class VulkanImage {
public:
  static void create(VkPhysicalDevice physical_device_handle,
                     VkDevice logical_device_handle, uint32_t width,
                     uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags device_properties, VkImage &image,
                     VkDeviceMemory &mem_image) {
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
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    ZEPH_ENSURE(vkCreateImage(logical_device_handle, &image_info, nullptr,
                              &image) != VK_SUCCESS,
                "Couldn't create texture image");

    VkMemoryRequirements mem_requirements;

    vkGetImageMemoryRequirements(logical_device_handle, image,
                                 &mem_requirements);

    VkMemoryAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = mem_requirements.size;
    allocate_info.memoryTypeIndex = VulkanPhysicalDevice::find_memory_type(
        physical_device_handle, mem_requirements.memoryTypeBits,
        device_properties);

    ZEPH_ENSURE(vkAllocateMemory(logical_device_handle, &allocate_info, nullptr,
                                 &mem_image) != VK_SUCCESS,
                "Couldn't allocate image memory");

    vkBindImageMemory(logical_device_handle, image, mem_image, 0);
  }

  static VkImageCreateInfo declare(VkImageUsageFlags usage, uint32_t width,
                                   uint32_t height, VkFormat format,
                                   VkImageTiling tiling) {

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
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    return image_info;
  }
};

} // namespace zephyr
