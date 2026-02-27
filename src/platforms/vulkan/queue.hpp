#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace zephyr {

struct VulkanQueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool exists() {
    return graphics_family.has_value() && present_family.has_value();
  }

  static VulkanQueueFamilyIndices find_queue_families(VkSurfaceKHR surface,
                                                      VkPhysicalDevice device,
                                                      uint32_t queues) {
    VulkanQueueFamilyIndices indices;

    uint32_t queue_family_count;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families.data());

    int i = 0;
    for (auto &queue_family : queue_families) {
      if (indices.exists())
        break;

      if (queue_family.queueFlags & queues) {
        indices.graphics_family = i;
      }

      VkBool32 is_present_support_available = false;

      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                           &is_present_support_available);

      if (is_present_support_available) {
        indices.present_family = i;
      }

      i++;
    }

    return indices;
  };
};

class VulkanQueue {
public:
  static VkSubmitInfo
  declare_submit(std::vector<VkCommandBuffer> &command_buffers) {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = command_buffers.size();
    submit_info.pCommandBuffers = command_buffers.data();

    return submit_info;
  }
};

} // namespace zephyr
