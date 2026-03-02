#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace zephyr {

struct VulkanCommandBuffer;

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
  static VkSubmitInfo declare_submit(std::vector<VkCommandBuffer> &handles) {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = handles.size();
    submit_info.pCommandBuffers = handles.data();

    return submit_info;
  }

  static void submit(VkQueue queue,
                     std::vector<VkCommandBuffer> &command_buffers,
                     bool wait_idle = false, VkFence fence = VK_NULL_HANDLE) {

    VkSubmitInfo submit_info = declare_submit(command_buffers);

    vkQueueSubmit(queue, 1, &submit_info, fence);

    if (wait_idle) {
      vkQueueWaitIdle(queue);
    }
  }

  static void submit(VkQueue queue, VkCommandBuffer &command_buffer,
                     bool wait_idle = false, VkFence fence = VK_NULL_HANDLE) {
    std::vector<VkCommandBuffer> command_buffers = {command_buffer};

    submit(queue, command_buffers, wait_idle, fence);
  }
};

} // namespace zephyr
