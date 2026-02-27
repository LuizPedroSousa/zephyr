#pragma once

#include "platforms/vulkan/device.hpp"
#include "platforms/vulkan/validation-layer.hpp"
#include <cstdint>
#include <set>
#include <vulkan/vulkan_core.h>

namespace zephyr {

VulkanLogicalDevice
VulkanLogicalDevice::create(const VulkanPhysicalDevice &physical_device) {
  VulkanLogicalDevice logical_device;

#ifdef ENABLE_VALIDATION_LAYER
  const std::vector<const char *> validation_layers = {
      "VK_LAYER_KHRONOS_validation"};
  ZEPH_ENSURE(
      !ValidationLayer::ensure_validation_layers_support(validation_layers),
      "Validation Layer not available");
#endif

  std::set<uint32_t> unique_queue_families = {
      physical_device.queue_family_indices.graphics_family.value(),
      physical_device.queue_family_indices.present_family.value(),
  };

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

  float queue_priority = 1.0f;

  for (uint32_t queue_family_index : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};

    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    queue_create_infos.emplace_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};

  VkDeviceCreateInfo create_info{};

  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());

  create_info.pEnabledFeatures = &device_features;

  create_info.enabledExtensionCount = physical_device.extensions.size();
  create_info.ppEnabledExtensionNames = physical_device.extensions.data();

#ifdef ENABLE_VALIDATION_LAYER
  create_info.enabledLayerCount = validation_layers.size();
  create_info.ppEnabledLayerNames = validation_layers.data();
#endif

  ZEPH_ENSURE(vkCreateDevice(physical_device.handle, &create_info, nullptr,
                             &logical_device.handle) != VK_SUCCESS,
              "Couldn't create logical device")

  logical_device.phy_handle = physical_device.handle;

  vkGetDeviceQueue(logical_device.handle,
                   physical_device.queue_family_indices.graphics_family.value(),
                   0, &logical_device.graphics_queue);

  ZEPH_ENSURE(logical_device.graphics_queue == VK_NULL_HANDLE,
              "Couldn't create queue for logical device");

  vkGetDeviceQueue(logical_device.handle,
                   physical_device.queue_family_indices.present_family.value(),
                   0, &logical_device.present_queue);

  ZEPH_ENSURE(logical_device.present_queue == VK_NULL_HANDLE,
              "Couldn't create queue for logical device");

  return logical_device;
}

} // namespace zephyr
